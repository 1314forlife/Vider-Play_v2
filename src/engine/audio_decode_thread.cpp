#include "audio_decode_thread.h"
#include "src/plugins/audio_decoder/audio_decoder.h"
#include "src/common/logger.h"
#include <QThread>

AudioDecodeThread::AudioDecodeThread(QObject* parent) : QThread(parent) {
}

AudioDecodeThread::~AudioDecodeThread() {
    stopThread();
}

void AudioDecodeThread::startThread() {
    if (isRunning()) return;
    m_running = true;
    m_paused = false;
    start();
    LOG_INFO("AudioDecodeThread", "音频解码线程启动");
}

void AudioDecodeThread::stopThread() {
    m_running = false;
    if (isRunning()) {
        wait();
    }
    LOG_INFO("AudioDecodeThread", "音频解码线程停止");
}

void AudioDecodeThread::pauseThread() {
    m_paused = true;
    LOG_INFO("AudioDecodeThread", "音频解码线程暂停");
}

void AudioDecodeThread::resumeThread() {
    m_paused = false;
    LOG_INFO("AudioDecodeThread", "音频解码线程恢复");
}

void AudioDecodeThread::flushDecoder() {
    if (m_decoder) {
        m_decoder->flush();
    }
    LOG_INFO("AudioDecodeThread", "已请求刷新解码器");
}

void AudioDecodeThread::run() {
    if (!m_decoder || !m_outputQueue) {
        LOG_ERROR("AudioDecodeThread", "解码器或输出队列未设置");
        return;
    }

    LOG_INFO("AudioDecodeThread", "音频解码线程开始运行");

    int frameCount = 0;
    int emptyCount = 0;
    int noPacketCount = 0;  // 🔥 新增：包队列空计数

    while (m_running) {
        try {
            if (m_paused) {
                QThread::msleep(10);
                continue;
            }

            if (m_outputQueue->size() >= 60) {
                QThread::msleep(5);
                continue;
            }

            // 🔥 新增：检查解码器的包队列是否有数据
            if (m_decoder->getPacketQueueSize() == 0) {
                noPacketCount++;
                if (noPacketCount % 100 == 0) {
                    LOG_DEBUG("AudioDecodeThread", QString("解码器包队列为空，已等待 %1 次，输出队列: %2")
                                                       .arg(noPacketCount).arg(m_outputQueue->size()));
                }
                QThread::msleep(1);
                continue;
            }
            noPacketCount = 0;

            AudioFrame frame;
            if (m_decoder->decodeFrame(frame)) {
                frameCount++;
                if (frameCount % 10 == 0) {
                    LOG_DEBUG("AudioDecodeThread", QString("已解码音频帧: %1, 输出队列: %2")
                                                       .arg(frameCount).arg(m_outputQueue->size()));
                }

                if (!frame.isValid()) {
                    LOG_WARN("AudioDecodeThread", "解码出无效音频帧，跳过");
                    continue;
                }

                m_outputQueue->push(std::move(frame));
                emit sigFrameDecoded();
            } else {
                emptyCount++;
                if (emptyCount % 100 == 0) {
                    LOG_DEBUG("AudioDecodeThread", QString("解码返回空，已尝试 %1 次，输出队列: %2")
                                                       .arg(emptyCount).arg(m_outputQueue->size()));
                }

                if (m_decoder->isEOF()) {
                    LOG_INFO("AudioDecodeThread", "音频解码完成");
                    emit sigDecodeFinished();
                    break;
                }
                QThread::msleep(1);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("AudioDecodeThread", QString("标准异常: %1").arg(e.what()));
        } catch (...) {
            LOG_ERROR("AudioDecodeThread", "未知异常");
        }
    }

    LOG_INFO("AudioDecodeThread", QString("音频解码线程退出，共解码 %1 帧").arg(frameCount));
}