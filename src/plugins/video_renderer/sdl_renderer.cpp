#include "sdl_renderer.h"
#include "src/common/logger.h"
#include <QString>

SDLRenderer::SDLRenderer(QObject* parent) : VideoRenderer(parent) {
    static bool sdlInit = false;
    if (!sdlInit) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            QString errorMsg = QString("SDL_Init failed: %1").arg(SDL_GetError());
            LOG_ERROR("SDLRenderer", errorMsg);
        }
        sdlInit = true;
    }
}

SDLRenderer::~SDLRenderer() {
    shutdown();
}

bool SDLRenderer::init(void* windowId, int videoWidth, int videoHeight) {
    if (m_initialized) shutdown();

    m_windowId = windowId;
    m_videoWidth = videoWidth;
    m_videoHeight = videoHeight;

    SDL_Window* window = SDL_CreateWindowFrom(windowId);
    if (!window) {
        QString errorMsg = QString("SDL_CreateWindowFrom failed: %1").arg(SDL_GetError());
        LOG_ERROR("SDLRenderer", errorMsg);
        return false;
    }
    m_window = window;

    SDL_GetWindowSize((SDL_Window*)m_window, &m_displayWidth, &m_displayHeight);

    // 尝试不同渲染驱动
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
    m_renderer = SDL_CreateRenderer((SDL_Window*)m_window, -1, SDL_RENDERER_ACCELERATED);

    if (!m_renderer) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d9");
        m_renderer = SDL_CreateRenderer((SDL_Window*)m_window, -1, SDL_RENDERER_ACCELERATED);
    }

    if (!m_renderer) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
        m_renderer = SDL_CreateRenderer((SDL_Window*)m_window, -1, SDL_RENDERER_ACCELERATED);
    }

    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer((SDL_Window*)m_window, -1, SDL_RENDERER_SOFTWARE);
        LOG_WARN("SDLRenderer", "使用软件渲染");
    }

    // 打印渲染器信息
    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_renderer, &info);
    LOG_INFO("SDLRenderer", QString("渲染驱动: %1, 硬件加速: %2")
                                .arg(info.name)
                                .arg((info.flags & SDL_RENDERER_ACCELERATED) ? "是" : "否"));

    // 关闭垂直同步（使用旧版 API）
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    createTexture();

    m_initialized = true;
    QString infoMsg = QString("SDLRenderer初始化成功: %1x%2").arg(videoWidth).arg(videoHeight);
    LOG_INFO("SDLRenderer", infoMsg);

    return true;
}

bool SDLRenderer::renderYUV(uint8_t* y, int yLinesize,
                            uint8_t* u, int uLinesize,
                            uint8_t* v, int vLinesize,
                            int64_t pts) {
    if (!m_initialized) return false;

    m_lastY = y;
    m_lastU = u;
    m_lastV = v;
    m_lastYLinesize = yLinesize;
    m_lastULinesize = uLinesize;
    m_lastVLinesize = vLinesize;

    if (!ensureContext()) {
        LOG_ERROR("SDLRenderer", "上下文恢复失败");
        return false;
    }

    int ret = SDL_UpdateYUVTexture(m_texture, nullptr,
                                   y, yLinesize,
                                   u, uLinesize,
                                   v, vLinesize);
    if (ret != 0) {
        LOG_WARN("SDLRenderer", "纹理更新失败，重新创建");
        createTexture();
        ret = SDL_UpdateYUVTexture(m_texture, nullptr,
                                   y, yLinesize,
                                   u, uLinesize,
                                   v, vLinesize);
        if (ret != 0) {
            QString errorMsg = QString("SDL_UpdateYUVTexture failed: %1").arg(SDL_GetError());
            LOG_ERROR("SDLRenderer", errorMsg);
            return false;
        }
    }

    // 🔥 优化：使用缓存的目标矩形，只在窗口大小改变时重新计算
    static SDL_Rect cachedDestRect;
    static int lastWidth = 0;
    static int lastHeight = 0;

    if (m_displayWidth != lastWidth || m_displayHeight != lastHeight) {
        // 只在窗口大小改变时重新计算
        float videoAspect = (float)m_videoWidth / m_videoHeight;
        float displayAspect = (float)m_displayWidth / m_displayHeight;

        if (displayAspect > videoAspect) {
            cachedDestRect.h = m_displayHeight;
            cachedDestRect.w = (int)(m_displayHeight * videoAspect);
            cachedDestRect.x = (m_displayWidth - cachedDestRect.w) / 2;
            cachedDestRect.y = 0;
        } else {
            cachedDestRect.w = m_displayWidth;
            cachedDestRect.h = (int)(m_displayWidth / videoAspect);
            cachedDestRect.x = 0;
            cachedDestRect.y = (m_displayHeight - cachedDestRect.h) / 2;
        }

        lastWidth = m_displayWidth;
        lastHeight = m_displayHeight;
    }

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, &cachedDestRect);
    SDL_RenderPresent(m_renderer);

    return true;
}

void SDLRenderer::resize(int width, int height) {
    if (!m_initialized) return;

    m_displayWidth = width;
    m_displayHeight = height;

    LOG_DEBUG("SDLRenderer", QString("窗口大小改变: %1x%2").arg(width).arg(height));
    // 🔥 直接渲染最后一帧（不通过 renderYUV，避免递归调用 resize）

}

void SDLRenderer::shutdown() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    m_initialized = false;
    m_lastY = nullptr;
    m_lastU = nullptr;
    m_lastV = nullptr;
}

void SDLRenderer::createTexture() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    m_texture = SDL_CreateTexture(m_renderer,
                                  SDL_PIXELFORMAT_IYUV,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  m_videoWidth, m_videoHeight);
    if (!m_texture) {
        QString errorMsg = QString("SDL_CreateTexture failed: %1").arg(SDL_GetError());
        LOG_ERROR("SDLRenderer", errorMsg);
    }
}

bool SDLRenderer::ensureContext() {
    if (!m_initialized) return false;

    bool needRebuild = false;

    if (!m_window) {
        LOG_WARN("SDLRenderer", "窗口丢失，重新创建");
        m_window = SDL_CreateWindowFrom(m_windowId);
        if (!m_window) return false;
        needRebuild = true;
    }

    if (!m_renderer) {
        LOG_WARN("SDLRenderer", "渲染器丢失，重新创建");
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        if (!m_renderer) return false;
        needRebuild = true;
    }

    if (needRebuild) {
        // 🔥 移除 SDL_RenderSetLogicalSize，因为现在手动计算显示区域
        // SDL_RenderSetLogicalSize(m_renderer, m_videoWidth, m_videoHeight);

        if (m_texture) {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }
        createTexture();

        // 🔥 重新渲染最后一帧（使用手动计算的位置）
        if (m_lastY && m_lastU && m_lastV && m_texture) {
            SDL_UpdateYUVTexture(m_texture, nullptr,
                                 m_lastY, m_lastYLinesize,
                                 m_lastU, m_lastULinesize,
                                 m_lastV, m_lastVLinesize);

            // 手动计算显示区域
            SDL_Rect destRect;
            float videoAspect = (float)m_videoWidth / m_videoHeight;
            float displayAspect = (float)m_displayWidth / m_displayHeight;

            if (displayAspect > videoAspect) {
                destRect.h = m_displayHeight;
                destRect.w = (int)(m_displayHeight * videoAspect);
                destRect.x = (m_displayWidth - destRect.w) / 2;
                destRect.y = 0;
            } else {
                destRect.w = m_displayWidth;
                destRect.h = (int)(m_displayWidth / videoAspect);
                destRect.x = 0;
                destRect.y = (m_displayHeight - destRect.h) / 2;
            }

            SDL_RenderClear(m_renderer);
            SDL_RenderCopy(m_renderer, m_texture, nullptr, &destRect);
            SDL_RenderPresent(m_renderer);
        }
    }

    if (!m_texture) {
        LOG_WARN("SDLRenderer", "纹理丢失，重新创建");
        createTexture();
        if (!m_texture) return false;
    }

    return true;
}