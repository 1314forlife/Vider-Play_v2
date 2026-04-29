#include "demux_thread.h"
#include "src/common/logger.h"
#include <QThread>

DemuxThread::DemuxThread(QObject* parent) : QThread(parent) {
    static bool init = false;
    if (!init) {
        avformat_network_init();
        init = true;
    }
}

DemuxThread::~DemuxThread() {
    stopThread();
    close();
}

bool DemuxThread::open(const QString& filePath) {
    close();

    // ========== 新增：判断是否为网络流 ==========
    m_isNetworkStream = filePath.startsWith("tcp://") || filePath.startsWith("http://");
    LOG_INFO("DemuxThread", QString("打开文件: %1, 是否为网络流: %2").arg(filePath).arg(m_isNetworkStream));

    std::string path = filePath.toStdString();

    // ========== 新增：网络流增加超时参数 ==========
    AVDictionary* opts = nullptr;
    if (m_isNetworkStream) {
        av_dict_set(&opts, "timeout", "30000000", 0);      // 30秒超时
        av_dict_set(&opts, "buffer_size", "2048000", 0);   // 2MB缓冲
        av_dict_set(&opts, "analyzeduration", "5000000", 0); // 分析时长5秒
        av_dict_set(&opts, "probesize", "5000000", 0);      // 探测大小5MB
        LOG_INFO("DemuxThread", "网络流: 设置超时和缓冲参数");
    }

    if (avformat_open_input(&m_formatCtx, path.c_str(), nullptr, &opts) < 0) {
        emit sigError("无法打开文件");
        if (opts) av_dict_free(&opts);
        return false;
    }
    if (opts) av_dict_free(&opts);

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        emit sigError("无法找到流信息");
        close();
        return false;
    }

    // 查找视频流和音频流，并保存参数
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        AVStream* stream = m_formatCtx->streams[i];
        AVCodecParameters* codecPar = stream->codecpar;

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) {
            m_videoStreamIndex = i;
            m_width = codecPar->width;
            m_height = codecPar->height;
            m_videoTimeBase = stream->time_base;

            // ========== 新增：视频流详细信息日志 ==========
            LOG_INFO("DemuxThread", QString("找到视频流，索引: %1").arg(i));
            LOG_INFO("DemuxThread", QString("codecpar->width = %1, height = %2").arg(m_width).arg(m_height));
            LOG_INFO("DemuxThread", QString("codecpar->codec_id = %1").arg(codecPar->codec_id));

            // 保存视频流参数
            m_videoCodecPar = avcodec_parameters_alloc();
            avcodec_parameters_copy(m_videoCodecPar, codecPar);

            if (stream->avg_frame_rate.den > 0) {
                m_fps = av_q2d(stream->avg_frame_rate);
            }

        } else if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) {
            m_audioStreamIndex = i;
            m_sampleRate = codecPar->sample_rate;
            m_channels = codecPar->ch_layout.nb_channels;
            m_audioTimeBase = stream->time_base;

            LOG_INFO("DemuxThread", QString("找到音频流，索引: %1, 采样率: %2, 通道: %3")
                                        .arg(i).arg(m_sampleRate).arg(m_channels));

            // 保存音频流参数
            m_audioCodecPar = avcodec_parameters_alloc();
            avcodec_parameters_copy(m_audioCodecPar, codecPar);
        }
    }

    if (m_formatCtx->duration != AV_NOPTS_VALUE) {
        m_duration = m_formatCtx->duration;
    }

    // ========== 新增：最终尺寸检查和默认值 ==========
    if (m_width == 0 || m_height == 0) {
        LOG_WARN("DemuxThread", QString("获取到无效尺寸 %1x%2，使用默认值 1280x720")
                                    .arg(m_width).arg(m_height));
        m_width = 1280;
        m_height = 720;
    }

    LOG_INFO("DemuxThread", QString("最终视频流: %1x%2, fps=%3, 音频流: %4Hz, %5通道")
                                .arg(m_width).arg(m_height).arg(m_fps)
                                .arg(m_sampleRate).arg(m_channels));

    emit sigStreamsReady();
    return true;
}

void DemuxThread::close() {
    if (m_videoCodecPar) {
        avcodec_parameters_free(&m_videoCodecPar);
        m_videoCodecPar = nullptr;
    }
    if (m_audioCodecPar) {
        avcodec_parameters_free(&m_audioCodecPar);
        m_audioCodecPar = nullptr;
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_eof = false;
}

bool DemuxThread::seek(int64_t timestampUs)
{
    if (!m_formatCtx) {
        LOG_ERROR("DemuxThread", "格式上下文未初始化");
        return false;
    }

    // 转换为 AVStream 的时间基
    int64_t timestamp;
    if (m_videoStreamIndex >= 0) {
        AVStream* stream = m_formatCtx->streams[m_videoStreamIndex];
        timestamp = av_rescale_q(timestampUs, {1, 1000000}, stream->time_base);
        int ret = av_seek_frame(m_formatCtx, m_videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            QString errorMsg = QString("Seek 失败: %1").arg(ret);
            LOG_ERROR("DemuxThread", errorMsg);
            return false;
        }
    } else if (m_audioStreamIndex >= 0) {
        AVStream* stream = m_formatCtx->streams[m_audioStreamIndex];
        timestamp = av_rescale_q(timestampUs, {1, 1000000}, stream->time_base);
        int ret = av_seek_frame(m_formatCtx, m_audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            QString errorMsg = QString("Seek 失败: %1").arg(ret);
            LOG_ERROR("DemuxThread", errorMsg);
            return false;
        }
    } else {
        return false;
    }

    // 清空解码器缓冲区
    m_eof = false;

    LOG_INFO("DemuxThread", QString("Seek 成功: %1 us").arg(timestampUs));
    return true;
}

void DemuxThread::startThread() {
    if (isRunning()) return;
    m_running = true;
    m_paused = false;
    m_eof = false;
    start();
    LOG_INFO("DemuxThread", "解复用线程启动");
}

void DemuxThread::stopThread() {
    m_running = false;
    if (isRunning()) {
        wait();
    }
    LOG_INFO("DemuxThread", "解复用线程停止");
}

void DemuxThread::pauseThread() {
    m_paused = true;
    LOG_INFO("DemuxThread", "解复用线程暂停");
}

void DemuxThread::resumeThread() {
    m_paused = false;
    LOG_INFO("DemuxThread", "解复用线程恢复");
}

void DemuxThread::flush() {
    // 清空内部缓冲（FFmpeg 格式上下文没有 flush，但我们可以重置 EOF）
    m_eof = false;
    LOG_INFO("DemuxThread", "解复用线程已刷新");
}

void DemuxThread::run() {
    if (!m_formatCtx) {
        LOG_ERROR("DemuxThread", "格式上下文未初始化");
        return;
    }

    if (!m_videoPacketQueue || !m_audioPacketQueue) {
        LOG_ERROR("DemuxThread", "包队列未设置");
        return;
    }

    LOG_INFO("DemuxThread", "解复用线程开始运行");

    AVPacket* packet = av_packet_alloc();
    static int audioPacketCount = 0;

    while (m_running && !m_eof) {
        if (m_paused) {
            QThread::msleep(10);
            continue;
        }

        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                m_eof = true;
                LOG_INFO("DemuxThread", "文件读取完毕");
                break;
            }
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_WARN("DemuxThread", QString("av_read_frame 错误: %1").arg(errbuf));
            continue;
        }

        Packet pkt = Packet::fromAVPacket(packet);

        // ============================
        // ✅【修复】音频包：必须优先投递！绝不等待！
        // ============================
        if (packet->stream_index == m_audioStreamIndex) {
            audioPacketCount++;
            if (audioPacketCount % 30 == 0) {
                LOG_DEBUG("DemuxThread", QString("读取音频包 #%1, 队列大小: %2")
                                             .arg(audioPacketCount).arg(m_audioPacketQueue->size()));
            }

            // 音频只丢不弃，绝不阻塞解复用
            if (m_audioPacketQueue->size() < 200) {
                m_audioPacketQueue->push(std::move(pkt));
            }

            av_packet_unref(packet);
            continue; // ✅ 关键：音频处理完直接进入下一帧，不碰视频逻辑
        }

        // ============================
        // ✅【修复】视频包：满了就丢帧，绝不卡住整个解复用！
        // ============================
        if (packet->stream_index == m_videoStreamIndex) {
            // 视频队列满 → 直接丢帧，不阻塞！
            if (m_videoPacketQueue->size() < 100) {
                m_videoPacketQueue->push(std::move(pkt));
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    LOG_INFO("DemuxThread", "解复用线程退出");
}