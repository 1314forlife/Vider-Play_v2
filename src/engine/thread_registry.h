#ifndef THREAD_REGISTRY_H
#define THREAD_REGISTRY_H

#include <QMap>
#include <QString>
#include <QThread>
#include <functional>
#include <memory>
#include "src/engine/audio_decode_thread.h"
#include "src/engine/video_decode_thread.h"
#include "src/engine/demux_thread.h"  // 🔥 添加

enum class ThreadType {
    Demux,          // 解复用线程
    VideoDecode,    // 视频解码线程
    AudioDecode,    // 音频解码线程
    VideoRender,    // 视频渲染线程
    AudioRender,    // 音频渲染线程
    Sync            // 同步线程
};

class ThreadRegistry {
public:
    using ThreadFactory = std::function<QThread*()>;

    static ThreadRegistry& instance() {
        static ThreadRegistry registry;
        return registry;
    }

    void registerThread(ThreadType type, ThreadFactory factory) {
        m_factories[type] = factory;
    }

    QThread* createThread(ThreadType type) {
        if (m_factories.contains(type)) {
            return m_factories[type]();
        }
        return nullptr;
    }

    // 🔥 初始化所有默认线程
    void initDefaultThreads() {
        registerThread(ThreadType::VideoDecode, []() {
            return new VideoDecodeThread();
        });

        registerThread(ThreadType::AudioDecode, []() {
            return new AudioDecodeThread();
        });

        registerThread(ThreadType::Demux, []() {
            return new DemuxThread();
        });
    }

private:
    QMap<ThreadType, ThreadFactory> m_factories;
};

#endif