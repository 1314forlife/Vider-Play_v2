#ifndef AUDIO_DECODE_THREAD_H
#define AUDIO_DECODE_THREAD_H

#include <QThread>
#include <atomic>
#include <memory>
#include "src/common/thread_safe_queue.h"
#include "src/core/audio_frame.h"
#include "src/plugins/audio_decoder/audio_decoder.h"



class AudioDecodeThread : public QThread {
    Q_OBJECT
public:
    explicit AudioDecodeThread(QObject* parent = nullptr);
    ~AudioDecodeThread();

    void setDecoder(AudioDecoder* decoder) { m_decoder = decoder; }
    void setOutputQueue(ThreadSafeQueue<AudioFrame>* queue) { m_outputQueue = queue; }

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
    AudioDecoder* m_decoder = nullptr;
    ThreadSafeQueue<AudioFrame>* m_outputQueue = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
};

#endif