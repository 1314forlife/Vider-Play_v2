#ifndef AUDIO_RENDERER_H
#define AUDIO_RENDERER_H

#include <QObject>
#include <cstdint>
#include "src/common/thread_safe_queue.h"  // 🔥 添加这行
#include "src/core/audio_frame.h"          // 🔥 添加这行

class AudioFrame;

class AudioRenderer : public QObject {
    Q_OBJECT
public:
    explicit AudioRenderer(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~AudioRenderer() = default;

    virtual bool init(int sampleRate, int channels) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void setVolume(int volume) = 0;  // 0-100

    // 设置音频数据队列
    virtual void setAudioQueue(class ThreadSafeQueue<AudioFrame>* queue) = 0;

signals:
    void sigError(const QString& error);
};

#endif