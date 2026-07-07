#include "video_decode_thread.h"
#include "src/common/logger.h"
#include <QThread>
#include <QElapsedTimer>

VideoDecodeThread::VideoDecodeThread(QObject* parent) : QThread(parent) {
}

VideoDecodeThread::~VideoDecodeThread() {
    stopThread();
}

void VideoDecodeThread::startThread() {
    if (isRunning()) return;
    m_running = true;
    m_paused = false;
    start();
    LOG_INFO("VideoDecodeThread", "解码线程启动");
}

void VideoDecodeThread::stopThread() {
    m_running = false;
    if (isRunning()) {
        wait();
    }
    LOG_INFO("VideoDecodeThread", "解码线程停止");
}

void VideoDecodeThread::pauseThread() {
    m_paused = true;
    LOG_INFO("VideoDecodeThread", "解码线程暂停");
}

void VideoDecodeThread::resumeThread() {
    m_paused = false;
    LOG_INFO("VideoDecodeThread", "解码线程恢复");
}

void VideoDecodeThread::flushDecoder() {
    if (m_decoder) {
        m_decoder->flush();
    }
    LOG_INFO("VideoDecodeThread", "已请求刷新视频解码器");
}

void VideoDecodeThread::run() {
    if (!m_decoder || !m_outputQueue) {
        LOG_ERROR("VideoDecodeThread", "解码器或输出队列未设置");
        return;
    }

    LOG_INFO("VideoDecodeThread", "解码线程开始运行 [诊断版]");
    QThread::currentThread()->setPriority(QThread::HighPriority);

    int decodedCount = 0;
    QElapsedTimer debugTimer;
    debugTimer.start();

    while (m_running) {
        if (m_paused) {
            QThread::msleep(10);
            continue;
        }

        // 1. 队列满限制（保持原样，防止内存爆掉）
        if (m_outputQueue->size() >= 5) {
            QThread::msleep(2); // 稍微缩短等待，提高响应性
            continue;
        }

        FrameData frame;

        // 🚨 监控点：如果卡死，这里会是最后的绝唱
        // LOG_DEBUG("VideoDecodeThread", "准备进入 decodeFrame...");

        bool success = m_decoder->decodeFrame(frame);

        if (success) {
            if (frame.isValid()) {
                m_outputQueue->push(std::move(frame));
                emit sigFrameDecoded();
                decodedCount++;
            }
        } else {
            if (m_decoder->isEOF()) {
                LOG_INFO("VideoDecodeThread", "视频解码器报 EOF，准备退出");
                emit sigDecodeFinished();
                break;
            }
            // 如果没解出来包，稍微歇 2ms，避免打满 CPU
            QThread::msleep(2);
        }

        // 每秒诊断输出
        if (debugTimer.elapsed() >= 1000) {
            LOG_INFO("VideoDecodeThread",
                     QString("[解码诊断] 本秒解码帧数: %1, 缓存队列大小: %2")
                         .arg(decodedCount).arg(m_outputQueue->size()));
            decodedCount = 0;
            debugTimer.restart();
        }
    }

    LOG_INFO("VideoDecodeThread", "解码线程退出");
}