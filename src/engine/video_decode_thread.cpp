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

void VideoDecodeThread::run() {
    if (!m_decoder || !m_outputQueue) {
        LOG_ERROR("VideoDecodeThread", "解码器或输出队列未设置");
        return;
    }

    LOG_INFO("VideoDecodeThread", "解码线程开始运行");

    // 🔥 限速相关变量
    QElapsedTimer fpsTimer;
    fpsTimer.start();
    int frameCount = 0;
    double targetFps = 30.0;
    int maxFramesPerSecond = 33;  // 最多 33 帧/秒

    while (m_running) {
        if (m_paused) {
            QThread::msleep(10);
            continue;
        }

        // 🔥 限速
        if (fpsTimer.elapsed() >= 1000) {
            if (frameCount > maxFramesPerSecond) {
                int waitMs = 1000 / targetFps;
                QThread::msleep(waitMs);
            }
            frameCount = 0;
            fpsTimer.restart();
        }

        // 🔥 队列限制
        if (m_outputQueue->size() >= 5) {
            QThread::msleep(5);
            continue;
        }

        FrameData frame;
        if (m_decoder->decodeFrame(frame)) {
            if (frame.isValid()) {
                m_outputQueue->push(std::move(frame));
                emit sigFrameDecoded();
                frameCount++;
            }
        } else {
            if (m_decoder->isEOF()) {
                LOG_INFO("VideoDecodeThread", "解码完成");
                emit sigDecodeFinished();
                break;
            }
            QThread::msleep(1);
        }
    }

    LOG_INFO("VideoDecodeThread", "解码线程退出");
}