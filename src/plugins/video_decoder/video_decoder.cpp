#include "video_decoder.h"
#include "src/common/logger.h"

extern "C" {
#include <libavformat/avformat.h>  //  添加这行
}

VideoDecoder::VideoDecoder(QObject* parent) : DecoderBase(parent) {
    static bool init = false;
    if (!init) {
        avformat_network_init();
        init = true;
    }
}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(AVCodecParameters* codecPar, AVRational timeBase) {
    close();

    m_timeBase = timeBase;
    m_width = codecPar->width;
    m_height = codecPar->height;

    if (!initDecoder(codecPar)) {
        return false;
    }

    m_isOpen = true;
    m_eof = false;

    LOG_INFO("VideoDecoder", QString("视频解码器初始化成功: %1x%2").arg(m_width).arg(m_height));

    return true;
}

bool VideoDecoder::initDecoder(AVCodecParameters* codecPar) {
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit sigError("找不到视频解码器");
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        emit sigError("无法分配解码器上下文");
        return false;
    }

    if (avcodec_parameters_to_context(m_codecCtx, codecPar) < 0) {
        emit sigError("无法复制解码器参数");
        return false;
    }

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        emit sigError("无法打开视频解码器");
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        emit sigError("无法分配帧");
        return false;
    }

    return true;
}

int64_t VideoDecoder::rescalePts(int64_t pts, AVRational from, AVRational to) {
    if (pts == AV_NOPTS_VALUE) return AV_NOPTS_VALUE;
    return av_rescale_q(pts, from, to);
}

bool VideoDecoder::decodePacket(AVPacket* packet, FrameData& frame) {
    int ret = avcodec_send_packet(m_codecCtx, packet);
    if (ret < 0) return false;

    ret = avcodec_receive_frame(m_codecCtx, m_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return false;
    if (ret < 0) return false;

    int64_t pts = m_frame->pts;
    if (pts != AV_NOPTS_VALUE) {
        pts = rescalePts(pts, m_timeBase, {1, 1000000});
    }

    //  移动：FrameData 接管 m_frame 的所有权
    frame = FrameData::fromAVFrame(m_frame, pts);

    // 重新分配新的 AVFrame 给下一帧使用
    m_frame = av_frame_alloc();

    return true;
}



bool VideoDecoder::seek(int64_t timestampUs) {
    // 解码器的 seek 现在由解复用线程处理
    // 这里只需要清空内部状态
    avcodec_flush_buffers(m_codecCtx);
    m_eof = false;
    return true;
}

void VideoDecoder::flush() {
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
    m_eof = false;
    LOG_INFO("VideoDecoder", "解码器缓冲区已刷新");
}
void VideoDecoder::close() {
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    m_isOpen = false;
    m_eof = false;
}

bool VideoDecoder::decodeFrame(FrameData &frame)
{
    if (!m_isOpen || m_eof) return false;
    if (!m_packetQueue) return false;

    //【步骤 1】：先尝试从解码器内部“榨取”已经解码好的、或者积压的帧
    int ret = avcodec_receive_frame(m_codecCtx, m_frame);
    if (ret == 0) {
        // 成功拿到了一帧！
        int64_t pts = m_frame->pts;
        if (pts != AV_NOPTS_VALUE) {
            pts = rescalePts(pts, m_timeBase, {1, 1000000});
        }
        frame = FrameData::fromAVFrame(m_frame, pts);
        m_frame = av_frame_alloc(); // 重新分配给下一帧用
        return true;
    }
    else if (ret == AVERROR_EOF) {
        m_eof = true;
        return false;
    }
    // 如果返回 EAGAIN，说明解码器空了，需要喂新数据，继续往下走

    //【步骤 2】：解码器空了，从外部队列里拿一个新包喂给它
    Packet packet;
    if (!m_packetQueue->tryPop(packet)) {
        return false; // 队列暂时没包，通常是因为 Demux 控流，等会再试
    }

    if (!packet.isValid()) {
        return false;
    }

    // 将包送入解码器
    ret = avcodec_send_packet(m_codecCtx, packet.get());
    if (ret < 0) {
        // 送入失败，可能是不可逆的错误
        LOG_ERROR("VideoDecoder", QString("avcodec_send_packet 失败, 错误码: %1").arg(ret));
        return false;
    }

    //【步骤 3】：喂完新包后，立刻再次尝试获取一次帧
    ret = avcodec_receive_frame(m_codecCtx, m_frame);
    if (ret == 0) {
        int64_t pts = m_frame->pts;
        if (pts != AV_NOPTS_VALUE) {
            pts = rescalePts(pts, m_timeBase, {1, 1000000});
        }
        frame = FrameData::fromAVFrame(m_frame, pts);
        m_frame = av_frame_alloc();
        return true;
    }

    if (ret == AVERROR(EAGAIN)) {
        // 这是完全正常的！说明这一包是 B 帧或者依赖帧，送进去了但还凑不齐一幅画
        // 返回 false 让线程下次循环继续来喂包，千万不要认为出错而退出线程！
        return false;
    }
    else if (ret == AVERROR_EOF) {
        m_eof = true;
    }

    return false;
}