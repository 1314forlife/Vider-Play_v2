#ifndef SDLRENDER_H
#define SDLRENDER_H

// 先包含基类，再包含SDL
#include "irender.h"
#include "frame_data.h"
class SDLRender : public IRender
{
    Q_OBJECT
public:
    explicit SDLRender(QObject *parent = nullptr);
    ~SDLRender() override;

    //实现IRender接口
    bool init(void* winId,int width, int height) override;
    Q_SLOT void render(const FrameData& frame) override;
    void shutdown() override;
    void setSize(int w, int h) override;

private:
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer*   m_sdlRenderer = nullptr;
    SDL_Texture*    m_sdlTexture = nullptr;
    int m_width = 0;
    int m_height = 0;
};
#endif // SDLRENDER_H
