#include "src/stdafx.h"
#include "sdlrender.h"
#include <QDebug>

SDLRender::SDLRender(QObject *parent) : IRender(parent)
{

}

SDLRender::~SDLRender()
{
    shutdown();
}

bool SDLRender::init(void* winId,int width, int height)
{
    //初始化SDL视频子系统
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        qDebug()<< "SDL init failed:" << SDL_GetError();
        return false;
    }
    // 绑定SDL窗口到Qt窗口句柄（关键！让SDL在Qt窗口内渲染）
    m_sdlWindow = SDL_CreateWindowFrom(winId);
    if(!m_sdlWindow){
        qDebug() << "Create SDL window failed:" << SDL_GetError();
        return false;
    }

    // 创建硬件加速渲染器
    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer) {
        qDebug() << "Create SDL renderer failed:" << SDL_GetError();
        return false;
    }

    m_width = width;
    m_height = height;

    // 创建YUV420P纹理（FFmpeg解码后直接用，无需RGB转换，性能拉满）
    m_sdlTexture = SDL_CreateTexture(
        m_sdlRenderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        m_width,
        m_height
        );

    if (!m_sdlTexture) {
        qDebug() << "Create SDL texture failed:" << SDL_GetError();
        return false;
    }

    return true;
}

void SDLRender::render(const FrameData& frame)
{
    if (!m_sdlTexture || !m_sdlRenderer || frame.data.size() < 3) return;

    SDL_UpdateYUVTexture(
    m_sdlTexture,
    nullptr,
    frame.data[0],frame.linesize[0],
    frame.data[1],frame.linesize[1],
    frame.data[2],frame.linesize[2]
    );

    //清屏，渲染，显示
    SDL_RenderClear(m_sdlRenderer);
    SDL_RenderCopy(m_sdlRenderer,m_sdlTexture,nullptr,nullptr);
    SDL_RenderPresent(m_sdlRenderer);
}

void SDLRender::shutdown()
{
    if(m_sdlTexture)SDL_DestroyTexture(m_sdlTexture);
    if(m_sdlRenderer)SDL_DestroyRenderer(m_sdlRenderer);
    if(m_sdlWindow)SDL_DestroyWindow(m_sdlWindow);
    SDL_Quit();

    m_sdlTexture = nullptr;
    m_sdlRenderer = nullptr;
    m_sdlWindow = nullptr;
}

void SDLRender::setSize(int w, int h)
{
    m_width = w;
    m_height = h;

    // 窗口缩放时，重新创建纹理（可选，也可以用SDL_RenderCopy自动适配）
    if (m_sdlTexture) {
        SDL_DestroyTexture(m_sdlTexture);
        m_sdlTexture = SDL_CreateTexture(
            m_sdlRenderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            m_width,
            m_height
            );
    }
}