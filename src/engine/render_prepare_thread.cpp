#include "render_prepare_thread.h"
#include "src/common/logger.h"
#include <QThread>

RenderPrepareThread::RenderPrepareThread(QObject* parent) : QThread(parent) {
}

RenderPrepareThread::~RenderPrepareThread() {
    stopThread();
}

void RenderPrepareThread::startThread() {
    if (isRunning()) return;
    m_running = true;
    m_paused = false;
    start();
    LOG_INFO("RenderPrepareThread", "渲染准备线程启动");
}

void RenderPrepareThread::stopThread() {
    m_running = false;
    if (isRunning()) {
        wait();
    }
    LOG_INFO("RenderPrepareThread", "渲染准备线程停止");
}

void RenderPrepareThread::pauseThread() {
    m_paused = true;
    LOG_INFO("RenderPrepareThread", "渲染准备线程暂停");
}

void RenderPrepareThread::resumeThread() {
    m_paused = false;
    LOG_INFO("RenderPrepareThread", "渲染准备线程恢复");
}

int64_t RenderPrepareThread::getAudioTime() {
    if (m_audioClock) {
        return m_audioClock->load();
    }
    return 0;
}

bool RenderPrepareThread::shouldDropFrame(int64_t framePts)
{
    int64_t audioTime = getAudioTime();
    if (audioTime <= 0) return false;

    int64_t diff = framePts - audioTime;

    // ✅ 网络流容错：增大阈值，避免音频“看起来滞后”
    // 视频最多比音频快 200ms 才丢帧（原来是几十毫秒）
    if (diff < -200000) {      // 视频超前 > 200ms
        return true;           // 丢帧
    }

    // 视频比音频慢，不丢帧（等音频）
    if (diff > 500000) {       // 视频落后 > 500ms
        return false;
    }

    return false;
}

void RenderPrepareThread::run() {
    if (!m_inputQueue) {
        LOG_ERROR("RenderPrepareThread", "输入队列未设置");
        return;
    }

    LOG_INFO("RenderPrepareThread", "渲染准备线程开始运行");

    // 提高优先级
    QThread::currentThread()->setPriority(QThread::HighPriority);

    int frameCount = 0;
    int dropCount = 0;
    QElapsedTimer timer;
    timer.start();

    // 🔥 获取双缓冲指针（需要从外部传入）
    // 注意：这些指针需要在 PlayEngine 中设置
    if (!m_renderCmds || !m_currRenderCmd || !m_writeCmdIdx) {
        LOG_ERROR("RenderPrepareThread", "双缓冲指针未设置");
        return;
    }

    while (m_running) {
        if (m_paused) {
            QThread::msleep(10);
            continue;
        }

        FrameData frame;
        if (m_inputQueue->tryPop(frame)) {
            if (!frame.isValid()) {
                continue;
            }

            frameCount++;

            // 🔥 双缓冲写入
            int writeIdx = m_writeCmdIdx->load();
            RenderCommand* cmd = &m_renderCmds[writeIdx];
            cmd->set(frame);

            // 原子交换，让主线程可以读取
            m_currRenderCmd->store(cmd);

            // 切换写索引
            m_writeCmdIdx->store(1 - writeIdx);

            // 等待主线程消费完成
            while (m_currRenderCmd->load() != nullptr) {
                QThread::usleep(100);
            }

            emit sigFrameReady();
        } else {
            QThread::usleep(100);
        }

        // 每秒输出统计
        if (timer.elapsed() >= 1000) {
            LOG_INFO("RenderPrepareThread",
                     QString("处理帧: %1, 丢帧: %2, 输入队列: %3")
                         .arg(frameCount).arg(dropCount)
                         .arg(m_inputQueue->size()));
            frameCount = 0;
            dropCount = 0;
            timer.restart();
        }
    }

    LOG_INFO("RenderPrepareThread", "渲染准备线程退出");
}