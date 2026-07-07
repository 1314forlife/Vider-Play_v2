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

    // 获取真实的渲染输出分辨率（完美兼容高分屏 DPI 缩放带来的左上角/黑边偏移）
    SDL_GetRendererOutputSize(m_renderer, &m_displayWidth, &m_displayHeight);
    m_lastWidth = m_displayWidth;
    m_lastHeight = m_displayHeight;

    // 打印渲染器信息
    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_renderer, &info);
    LOG_INFO("SDLRenderer", QString("渲染驱动: %1, 硬件加速: %2")
                                .arg(info.name)
                                .arg((info.flags & SDL_RENDERER_ACCELERATED) ? "是" : "否"));

    // 关闭垂直同步
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    // 开启双线性过滤，让缩放后的视频边缘和画质更平滑
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    createTexture();
    updateDestRect();

    m_initialized = true;
    QString infoMsg = QString("SDLRenderer初始化成功: %1x%2, 渲染大小: %3x%4")
                          .arg(videoWidth).arg(videoHeight).arg(m_displayWidth).arg(m_displayHeight);
    LOG_INFO("SDLRenderer", infoMsg);

    return true;
}

void SDLRenderer::updateDestRect() {
    if (m_displayWidth <= 0 || m_displayHeight <= 0 || m_videoWidth <= 0 || m_videoHeight <= 0) {
        return;
    }

    float videoAspect = (float)m_videoWidth / m_videoHeight;
    float displayAspect = (float)m_displayWidth / m_displayHeight;

    if (displayAspect > videoAspect) {
        m_destRect.h = m_displayHeight;
        m_destRect.w = (int)(m_displayHeight * videoAspect);
        m_destRect.x = (m_displayWidth - m_destRect.w) / 2;
        m_destRect.y = 0;
    } else {
        m_destRect.w = m_displayWidth;
        m_destRect.h = (int)(m_displayWidth / videoAspect);
        m_destRect.x = 0;
        m_destRect.y = (m_displayHeight - m_destRect.h) / 2;
    }
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

    // 动态检查外部是否通过 resize 改变了分辨率，双重保险
    if (m_displayWidth != m_lastWidth || m_displayHeight != m_lastHeight) {
        updateDestRect();
        m_lastWidth = m_displayWidth;
        m_lastHeight = m_displayHeight;
    }

    // 每一帧都要清屏，防止缩小时周围旧的缓存画面残留在黑边区域
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, &m_destRect);
    SDL_RenderPresent(m_renderer);

    return true;
}

void SDLRenderer::resize(int width, int height) {
    if (!m_initialized) return;

    // 绝大多数情况下 Qt 传过来的 width/height 是逻辑像素
    // 我们直接向 SDL 询问当前对应窗口在底层的真实硬件像素大小
    SDL_GetRendererOutputSize(m_renderer, &m_displayWidth, &m_displayHeight);

    // 更新目标视口坐标
    updateDestRect();

    LOG_DEBUG("SDLRenderer", QString("窗口大小改变，逻辑: %1x%2, 硬件渲染尺寸: %3x%4")
                                 .arg(width).arg(height).arg(m_displayWidth).arg(m_displayHeight));

    // 如果当前有最后一帧的缓存，立刻重绘，防止放大缩小过程中出现短暂的黑屏或画面闪烁
    if (m_lastY && m_lastU && m_lastV && m_texture) {
        SDL_RenderClear(m_renderer);
        SDL_RenderCopy(m_renderer, m_texture, nullptr, &m_destRect);
        SDL_RenderPresent(m_renderer);
    }
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
    m_destRect = {0, 0, 0, 0};
    m_lastWidth = 0;
    m_lastHeight = 0;
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
        SDL_GetRendererOutputSize(m_renderer, &m_displayWidth, &m_displayHeight);
        updateDestRect();

        if (m_texture) {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }
        createTexture();

        if (m_lastY && m_lastU && m_lastV && m_texture) {
            SDL_UpdateYUVTexture(m_texture, nullptr,
                                 m_lastY, m_lastYLinesize,
                                 m_lastU, m_lastULinesize,
                                 m_lastV, m_lastVLinesize);

            SDL_RenderClear(m_renderer);
            SDL_RenderCopy(m_renderer, m_texture, nullptr, &m_destRect);
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