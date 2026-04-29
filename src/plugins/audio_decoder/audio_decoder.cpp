#include "audio_decoder.h"
#include "src/common/logger.h"

AudioDecoder::AudioDecoder(QObject* parent) : AudioDecoderBase(parent) {
}

AudioDecoder::~AudioDecoder() {
    close();
}

void AudioDecoder::flush() {
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
    m_eof = false;
    LOG_INFO("AudioDecoder", "解码器缓冲区已刷新");
}
bool AudioDecoder::open(AVCodecParameters* codecPar, AVRational timeBase) {
    close();

    m_timeBase = timeBase;
    m_sampleRate = codecPar->sample_rate;
    m_channels = codecPar->ch_layout.nb_channels;

    if (!initDecoder(codecPar)) {
        return false;
    }

    m_isOpen = true;
    m_eof = false;

    LOG_INFO("AudioDecoder", QString("音频解码器初始化成功: %1Hz, %2通道")
                                 .arg(m_sampleRate).arg(m_channels));

    return true;
}

bool AudioDecoder::initDecoder(AVCodecParameters* codecPar) {
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit sigError("找不到音频解码器");
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
        emit sigError("无法打开音频解码器");
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        emit sigError("无法分配帧");
        return false;
    }

    // ==========================
    // 🔥 🔥 🔥 重采样初始化 已完全修复
    // ==========================
    if (!m_swrCtx) {
        // 输入：来自解码器
        AVChannelLayout in_ch_layout = m_codecCtx->ch_layout;
        AVSampleFormat    in_fmt      = m_codecCtx->sample_fmt;
        int              in_rate     = m_codecCtx->sample_rate;

        // 输出：SDL 要求的标准格式（16位 交错）
        AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO; // 立体声
        AVSampleFormat    out_fmt     = AV_SAMPLE_FMT_S16;
        int              out_rate     = in_rate; // 采样率保持不变

        // 正确调用 swr_alloc_set_opts2
        int ret = swr_alloc_set_opts2(&m_swrCtx,
                                      &out_ch_layout,      // 输出通道布局（指针！）
                                      out_fmt,             // 输出格式
                                      out_rate,            // 输出采样率

                                      &in_ch_layout,       // 输入通道布局（指针！）
                                      in_fmt,              // 输入格式
                                      in_rate,             // 输入采样率

                                      0, nullptr
                                      );

        // 分配失败
        if (ret < 0 || !m_swrCtx) {
            LOG_ERROR("AudioDecoder", "重采样器创建失败");
            return false;
        }

        // ✅ 必须调用 swr_init ！！！（你之前漏了这个）
        if (swr_init(m_swrCtx) < 0) {
            LOG_ERROR("AudioDecoder", "重采样器初始化失败");
            swr_free(&m_swrCtx);
            return false;
        }

        LOG_INFO("AudioDecoder", "重采样器初始化成功");
    }

    // 把正确的采样率、声道保存下来
    m_sampleRate = m_codecCtx->sample_rate;
    m_channels   = 2; // 重采样转成了双声道

    return true;
}

int64_t AudioDecoder::rescalePts(int64_t pts, AVRational from, AVRational to) {
    if (pts == AV_NOPTS_VALUE) return AV_NOPTS_VALUE;
    return av_rescale_q(pts, from, to);
}

bool AudioDecoder::decodePacket(AVPacket* packet, AudioFrame& frame) {
    int ret = avcodec_send_packet(m_codecCtx, packet);
    if (ret < 0) return false;

    ret = avcodec_receive_frame(m_codecCtx, m_frame);
    if (ret < 0) return false;

    // 🔥 计算重采样后的样本数
    int dst_nb_samples = av_rescale_rnd(m_frame->nb_samples, m_sampleRate, m_sampleRate, AV_ROUND_UP);

    // 🔥 分配重采样缓冲区
    if (!m_resampledData) {
        ret = av_samples_alloc_array_and_samples(&m_resampledData, &m_resampledLinesize,
                                                 m_channels, dst_nb_samples, AV_SAMPLE_FMT_S16, 0);
        if (ret < 0) {
            LOG_ERROR("AudioDecoder", "分配重采样缓冲区失败");
            return false;
        }
    }

    // 🔥 执行重采样
    int samples = swr_convert(m_swrCtx, m_resampledData, dst_nb_samples,
                              (const uint8_t**)m_frame->data, m_frame->nb_samples);

    if (samples > 0) {
        // 创建输出帧
        AVFrame* outFrame = av_frame_alloc();
        outFrame->format = AV_SAMPLE_FMT_S16;
        outFrame->sample_rate = m_sampleRate;
        outFrame->ch_layout.nb_channels = m_channels;
        outFrame->nb_samples = samples;
        av_frame_get_buffer(outFrame, 0);

        // 拷贝重采样后的数据
        memcpy(outFrame->data[0], m_resampledData[0], samples * m_channels * 2);

        int64_t pts = m_frame->pts;
        if (pts != AV_NOPTS_VALUE) {
            pts = rescalePts(pts, m_timeBase, {1, 1000000});
        }

        frame = AudioFrame::fromAVFrame(outFrame, pts);
        return true;
    }

    return false;
}
bool AudioDecoder::decodeFrame(AudioFrame& frame) {
    static int callCount = 0;
    static int successCount = 0;
    callCount++;

    if (!m_isOpen || m_eof) {
        LOG_DEBUG("AudioDecoder", QString("解码器未打开或已结束: isOpen=%1, eof=%2").arg(m_isOpen).arg(m_eof));
        return false;
    }

    if (!m_packetQueue) {
        LOG_DEBUG("AudioDecoder", "包队列为空指针");
        return false;
    }

    Packet packet;
    if (!m_packetQueue->tryPop(packet)) {
        if (callCount % 100 == 0) {
            LOG_DEBUG("AudioDecoder", QString("第 %1 次调用，包队列为空，队列大小: %2")
                                          .arg(callCount).arg(m_packetQueue->size()));
        }
        return false;
    }

    LOG_DEBUG("AudioDecoder", QString("成功取出包，包大小: %1, 有效: %2")
                                  .arg(packet.size()).arg(packet.isValid()));

    if (!packet.isValid()) {
        LOG_WARN("AudioDecoder", "包无效");
        return false;
    }

    if (decodePacket(packet.get(), frame)) {
        successCount++;
        if (successCount % 10 == 0) {
            LOG_DEBUG("AudioDecoder", QString("成功解码音频帧，总数: %1").arg(successCount));
        }
        return true;
    }

    LOG_WARN("AudioDecoder", "decodePacket 返回 false");
    return false;
}

void AudioDecoder::close() {
    if (m_resampledData) {
        av_freep(&m_resampledData[0]);
        av_freep(&m_resampledData);
        m_resampledData = nullptr;
        m_resampledLinesize = 0;
    }
    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }
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