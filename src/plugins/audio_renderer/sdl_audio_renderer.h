#ifndef SDL_AUDIO_RENDERER_H
#define SDL_AUDIO_RENDERER_H

#include "audio_renderer.h"
#include "src/common/thread_safe_queue.h"
#include "src/core/audio_frame.h"
#include <SDL2/SDL.h>
#include <atomic>

class SDLAudioRenderer : public AudioRenderer {
    Q_OBJECT
public:
    explicit SDLAudioRenderer(QObject* parent = nullptr);
    ~SDLAudioRenderer();

    bool init(int sampleRate, int channels) override;
    void play() override;
    void pause() override;
    void stop() override;
    void setVolume(int volume) override;
    void setAudioQueue(ThreadSafeQueue<AudioFrame>* queue) override;

private:
    // SDL 音频回调函数（静态）
    static void audioCallback(void* userdata, Uint8* stream, int len);

    // 实际回调处理
    void fillAudioData(Uint8* stream, int len);

    SDL_AudioDeviceID m_deviceId = 0;
    SDL_AudioSpec m_spec;
    bool m_needFreeBuffer = false;

    ThreadSafeQueue<AudioFrame>* m_audioQueue = nullptr;
    std::atomic<bool> m_playing{false};

    // 当前正在播放的音频数据
    uint8_t* m_currentBuffer = nullptr;
    int m_currentBufferSize = 0;
    int m_currentBufferPos = 0;

    int m_volume = 100;  // 0-100
    int m_sampleRate = 0;
    int m_channels = 0;
};

#endif