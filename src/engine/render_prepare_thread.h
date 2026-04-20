#ifndef RENDER_PREPARE_THREAD_H
#define RENDER_PREPARE_THREAD_H

#include <QThread>
#include <atomic>
#include <QElapsedTimer>
#include "src/common/thread_safe_queue.h"
#include "src/core/frame.h"

class RenderPrepareThread : public QThread {
    Q_OBJECT
public:
    explicit RenderPrepareThread(QObject* parent = nullptr);
    ~RenderPrepareThread();

    void setInputQueue(ThreadSafeQueue<FrameData>* queue) { m_inputQueue = queue; }
    // 🔥 删除 setOutputQueue，改为 setRenderCmds
    // void setOutputQueue(ThreadSafeQueue<FrameData>* queue) { m_outputQueue = queue; }

    // 🔥 新增：设置双缓冲
    void setRenderCmds(RenderCommand* cmds,
                       std::atomic<RenderCommand*>* currCmd,
                       std::atomic<int>* writeIdx) {
        m_renderCmds = cmds;
        m_currRenderCmd = currCmd;
        m_writeCmdIdx = writeIdx;
    }

    void startThread();
    void stopThread();
    void pauseThread();
    void resumeThread();

    // 音视频同步相关
    void setAudioClock(std::atomic<int64_t>* clock) { m_audioClock = clock; }
    void setTargetFps(double fps) { m_targetFps = fps; }

signals:
    void sigFrameReady();

protected:
    void run() override;

private:
    bool shouldDropFrame(int64_t framePts);
    int64_t getAudioTime();

    ThreadSafeQueue<FrameData>* m_inputQueue = nullptr;
    // 🔥 删除 m_outputQueue
    // ThreadSafeQueue<FrameData>* m_outputQueue = nullptr;

    // 🔥 新增：双缓冲成员
    RenderCommand* m_renderCmds = nullptr;
    std::atomic<RenderCommand*>* m_currRenderCmd = nullptr;
    std::atomic<int>* m_writeCmdIdx = nullptr;

    std::atomic<int64_t>* m_audioClock = nullptr;
    std::atomic<double> m_targetFps{30.0};

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};

    // 同步参数
    static constexpr double SYNC_THRESHOLD = 0.04;
    static constexpr double MAX_SYNC_DIFF = 0.5;
    static constexpr int64_t US_PER_SECOND = 1000000;
};

#endif