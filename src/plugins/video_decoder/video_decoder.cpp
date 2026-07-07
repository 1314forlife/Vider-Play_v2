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

    Packet packet;
    if (!m_packetQueue->tryPop(packet)) {
        return false;
    }

    if (!packet.isValid()) {
        return false;
    }

    if (decodePacket(packet.get(), frame)) {
        return true;
    }

    return false;
}