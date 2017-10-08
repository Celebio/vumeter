#include "rwqueuetype.hpp"
#include <SDL.h>

class TheDisplayer {
public:
    TheDisplayer(RWQueue *lockFreeQueue);
    ~TheDisplayer();
    void readAndDisplay();
private:
    RWQueue *m_lockFreeQueue;
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    void check_error_sdl(bool check, const char* message);
    void check_error_sdl_img(bool check, const char* message);
    SDL_Texture* load_texture(const char* fname, SDL_Renderer *renderer);
    double getLatestAverageFromQueue();
};
