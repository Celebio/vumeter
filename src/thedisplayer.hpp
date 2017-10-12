#include "rwqueuetype.hpp"
#include <SDL.h>
#include <memory>

using SDLWindowDestroyerType = void (*)(SDL_Window*);
using SDLRendererDestroyerType = void (*)(SDL_Renderer*);
using SDLTextureDestroyerType = void (*)(SDL_Texture*);


// Singleton that takes care of C-style resource : SDL_Init SDL_Quit
class SDLResource {
public:
    ~SDLResource();
    static SDLResource *getInstance();
private:
    static SDLResource *m_instance;
    SDLResource();
};


class TheDisplayer {
public:
    explicit TheDisplayer(RWQueue *lockFreeQueue);
    ~TheDisplayer();
    void readAndDisplay() const;
private:
    TheDisplayer(const TheDisplayer &);
    RWQueue *m_lockFreeQueue;
    std::unique_ptr<SDLResource> m_sdlResource;
    std::unique_ptr<SDL_Window, SDLWindowDestroyerType> m_window;
    std::unique_ptr<SDL_Renderer, SDLRendererDestroyerType> m_renderer;
    std::unique_ptr<SDL_Texture, SDLTextureDestroyerType> m_texture;
    double getLatestAverageFromQueue() const;
};
