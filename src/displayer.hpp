#ifndef DISPLAYER_HPP
#define DISPLAYER_HPP

#include "rwqueuetype.hpp"

#include <SDL.h>
#include <memory>
#include <vector>

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


class Displayer {
public:
    explicit Displayer(RWQueue *lockFreeQueue, RWVectorQueue *lockFreeVectorQueue);
    ~Displayer();
    void readAndDisplay();
private:
    Displayer(const Displayer &);
    RWQueue *m_lockFreeQueue;
    RWVectorQueue *m_lockFreeVectorQueue;
    std::unique_ptr<SDLResource> m_sdlResource;
    std::unique_ptr<SDL_Window, SDLWindowDestroyerType> m_window;
    std::unique_ptr<SDL_Renderer, SDLRendererDestroyerType> m_renderer;
    std::unique_ptr<SDL_Texture, SDLTextureDestroyerType> m_texture;
    std::vector<double> m_lastFrequencyAmplitudes;
    double m_level;
    void fetchLatestAverageFromQueue();
    void fetchLatestFrequencyAmplitudes();
};

#endif
