#include "sdl_audio_renderer.h"
#include "src/common/logger.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SDLAudioRenderer::SDLAudioRenderer(QObject* parent) : AudioRenderer(parent) {
    static bool sdlInit = false;
    if (!sdlInit) {
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            QString errorMsg = QString("SDL_Init Audio failed: %1").arg(SDL_GetError());
            LOG_ERROR("SDLAudioRenderer", errorMsg);
        }
        sdlInit = true;
    }
}

SDLAudioRenderer::~SDLAudioRenderer() {
    stop();
}

bool SDLAudioRenderer::init(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = sampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = channels;
    want.samples = 1024;
    want.callback = audioCallback;
    want.userdata = this;

    LOG_INFO("SDLAudioRenderer", QString("尝试打开音频设备: %1Hz, %2通道").arg(sampleRate).arg(channels));

    m_deviceId = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (m_deviceId == 0) {
        QString errorMsg = QString("打开音频设备失败: %1").arg(SDL_GetError());
        LOG_ERROR("SDLAudioRenderer", errorMsg);
        return false;
    }

    LOG_INFO("SDLAudioRenderer", QString("音频设备打开成功, deviceId=%1, 实际: %2Hz, %3通道")
                                     .arg(m_deviceId).arg(have.freq).arg(have.channels));

    // 🔥 确保设备是暂停状态（等待 play() 时启动）
    SDL_PauseAudioDevice(m_deviceId, 1);

    return true;
}

void SDLAudioRenderer::play() {
    bool playing = m_playing.load();
    LOG_INFO("SDLAudioRenderer", QString("play() 被调用, deviceId=%1, m_playing=%2")
                                     .arg(m_deviceId).arg(playing));

    if (m_deviceId != 0) {
        // 检查设备暂停状态
        // int paused = SDL_AudioDevicePaused(m_deviceId);
        // LOG_INFO("SDLAudioRenderer", QString("播放前设备暂停状态: %1 (1=暂停, 0=播放)").arg(paused));

        SDL_PauseAudioDevice(m_deviceId, 0);
        m_playing = true;

        // int afterPaused = SDL_AudioDevicePaused(m_deviceId);
        // LOG_INFO("SDLAudioRenderer", QString("播放后设备暂停状态: %1 (1=暂停, 0=播放)").arg(afterPaused));
    } else {
        LOG_ERROR("SDLAudioRenderer", "deviceId 为 0，无法播放");
    }
}

void SDLAudioRenderer::pause() {
    bool playing = m_playing.load();
    LOG_INFO("SDLAudioRenderer", QString("pause() 被调用, deviceId=%1, m_playing=%2")
                                     .arg(m_deviceId).arg(playing));

    if (m_deviceId != 0) {
        SDL_PauseAudioDevice(m_deviceId, 1);
        m_playing = false;
    }
}

void SDLAudioRenderer::stop() {
    if (m_needFreeBuffer && m_currentBuffer) {
        delete[] m_currentBuffer;
        m_currentBuffer = nullptr;
        m_needFreeBuffer = false;
    }
    if (m_deviceId != 0) {
        SDL_PauseAudioDevice(m_deviceId, 1);
        SDL_CloseAudioDevice(m_deviceId);
        m_deviceId = 0;
    }
    m_playing = false;
    m_currentBuffer = nullptr;
    m_currentBufferSize = 0;
    m_currentBufferPos = 0;
    LOG_INFO("SDLAudioRenderer", "音频设备关闭");
}

void SDLAudioRenderer::setVolume(int volume) {
    m_volume = std::max(0, std::min(100, volume));
}

void SDLAudioRenderer::setAudioQueue(ThreadSafeQueue<AudioFrame>* queue) {
    m_audioQueue = queue;
}

void SDLAudioRenderer::audioCallback(void* userdata, Uint8* stream, int len) {
    static int callCount = 0;
    callCount++;
    if (callCount == 1) {
        LOG_INFO("SDLAudioRenderer", "🎵 audioCallback 第一次被调用（正式播放时）！");
    }

    SDLAudioRenderer* renderer = static_cast<SDLAudioRenderer*>(userdata);
    renderer->fillAudioData(stream, len);
}

void SDLAudioRenderer::fillAudioData(Uint8* stream, int len) {
    static int callCount = 0;
    callCount++;
    if (callCount % 50 == 0) {
        LOG_DEBUG("SDLAudioRenderer", QString("音频回调 #%1, 长度: %2, 队列大小: %3")
                                          .arg(callCount).arg(len).arg(m_audioQueue ? m_audioQueue->size() : 0));
    }

    SDL_memset(stream, 0, len);

    if (!m_playing) return;
    if (!m_audioQueue) return;

    AudioFrame frame;
    if (m_audioQueue->tryPop(frame)) {
        if (frame.isValid()) {
            uint8_t** audioData = frame.data();
            if (audioData && audioData[0]) {
                int dataSize = frame.samples() * frame.channels() * 2;
                int copySize = (dataSize < len) ? dataSize : len;
                memcpy(stream, audioData[0], copySize);
            }
        }
    }
}