#ifndef SDL_PTR_H
#define SDL_PTR_H

#include <memory>
#include <SDL2/SDL.h>

// SDL_Window 智能指针删除器
struct SDL_WindowDeleter {
    void operator()(SDL_Window* window) {
        if (window) {
            SDL_DestroyWindow(window);
        }
    }
};

// SDL_Renderer 智能指针删除器
struct SDL_RendererDeleter {
    void operator()(SDL_Renderer* renderer) {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
    }
};

// SDL_Texture 智能指针删除器
struct SDL_TextureDeleter {
    void operator()(SDL_Texture* texture) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
};

// SDL_Surface 智能指针删除器
struct SDL_SurfaceDeleter {
    void operator()(SDL_Surface* surface) {
        if (surface) {
            SDL_FreeSurface(surface);
        }
    }
};

// 类型别名
using SDL_WindowPtr = std::unique_ptr<SDL_Window, SDL_WindowDeleter>;
using SDL_RendererPtr = std::unique_ptr<SDL_Renderer, SDL_RendererDeleter>;
using SDL_TexturePtr = std::unique_ptr<SDL_Texture, SDL_TextureDeleter>;
using SDL_SurfacePtr = std::unique_ptr<SDL_Surface, SDL_SurfaceDeleter>;

#endif // SDL_PTR_H