#ifndef VIDEO_DECODE_THREAD_H
#define VIDEO_DECODE_THREAD_H

#include <QThread>
#include <atomic>
#include <memory>
#include "src/plugins/video_decoder/video_decoder.h"
#include "src/common/thread_safe_queue.h"
#include "src/core/frame.h"

class VideoDecodeThread : public QThread {
    Q_OBJECT
public:
    explicit VideoDecodeThread(QObject* parent = nullptr);
    ~VideoDecodeThread();

    void setDecoder(VideoDecoder* decoder) { m_decoder = decoder; }
    void setOutputQueue(ThreadSafeQueue<FrameData>* queue) { m_outputQueue = queue; }

    void startThread();
    void stopThread();
    void pauseThread();
    void resumeThread();

    void flushDecoder();

signals:
    void sigFrameDecoded();
    void sigDecodeFinished();
    void sigError(const QString& error);

protected:
    void run() override;

private:
    VideoDecoder* m_decoder = nullptr;
    ThreadSafeQueue<FrameData>* m_outputQueue = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
};

#endif