#ifndef SDL_RENDERER_H
#define SDL_RENDERER_H

#include <cstdint>
#include <SDL2/SDL.h>  // 🔥 添加 SDL 头文件
#include "video_renderer.h"

class SDLRenderer : public VideoRenderer {
    Q_OBJECT
public:
    explicit SDLRenderer(QObject* parent = nullptr);
    ~SDLRenderer();

    bool init(void* windowId, int videoWidth, int videoHeight) override;
    bool renderYUV(uint8_t* y, int yLinesize,
                   uint8_t* u, int uLinesize,
                   uint8_t* v, int vLinesize,
                   int64_t pts) override;
    void resize(int width, int height) override;
    void shutdown() override;

private:
    void createTexture();
    bool ensureContext();

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    // 保存原始窗口句柄和视频参数
    void* m_windowId = nullptr;
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    int m_displayWidth = 0;
    int m_displayHeight = 0;
    bool m_initialized = false;

    // 缓存最后一帧数据
    uint8_t* m_lastY = nullptr;
    uint8_t* m_lastU = nullptr;
    uint8_t* m_lastV = nullptr;
    int m_lastYLinesize = 0;
    int m_lastULinesize = 0;
    int m_lastVLinesize = 0;

    void updateDestRect();
    SDL_Rect m_destRect = {0, 0, 0, 0};
    int m_lastWidth = 0;
    int m_lastHeight = 0;
};

#endif