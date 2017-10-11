#include "thedisplayer.hpp"

#include <SDL_image.h>
#include <iostream>
#include <queue>

using namespace std;

SDLResource* SDLResource::m_instance;

SDLResource::SDLResource(){
    SDL_Init(SDL_INIT_VIDEO);
}
SDLResource::~SDLResource(){
    std::cout << "Destroying SDLResource" << std::endl;
    IMG_Quit();
    SDL_Quit();
}

SDLResource *SDLResource::getInstance(){
    if (!SDLResource::m_instance)
        SDLResource::m_instance = new SDLResource();
    return SDLResource::m_instance;
}


void checkErrorSdlImg(bool check, const char* message) {
    if (check) {
        std::cout << message << " " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        std::exit(-1);
    }
}

SDL_Texture* loadTexture(const char* fname, SDL_Renderer *renderer) {
    SDL_Surface *image = IMG_Load(fname);
    checkErrorSdlImg(image == nullptr, "Unable to load image");

    SDL_Texture *img_texture = SDL_CreateTextureFromSurface(renderer, image);
    checkErrorSdlImg(img_texture == nullptr, "Unable to create a texture from the image");
    SDL_FreeSurface(image);
    return img_texture;
}


// From https://eb2.co/blog/2014/04/c--14-and-sdl2-managing-resources/
template<typename Creator, typename Destructor, typename... Arguments>
auto makeResource(Creator c, Destructor d, Arguments&&... args)
{
    auto r = c(std::forward<Arguments>(args)...);
    if (!r) { throw std::system_error(errno, std::generic_category()); }
    return std::unique_ptr<std::decay_t<decltype(*r)>, decltype(d)>(r, d);
}


TheDisplayer::TheDisplayer(RWQueue *lockFreeQueue) :
    m_lockFreeQueue(lockFreeQueue),
    m_sdlResource(SDLResource::getInstance()),
    m_window(makeResource(SDL_CreateWindow, SDL_DestroyWindow, "Sebastien", 0, 0, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)),
    m_renderer(makeResource(SDL_CreateRenderer, SDL_DestroyRenderer, m_window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)),
    m_texture(makeResource(loadTexture, SDL_DestroyTexture, "img_test.png", m_renderer.get()))
{
    SDL_Renderer *renderer = m_renderer.get();
    SDL_SetRenderDrawColor(renderer, 100, 149, 237, 255);

    int flags=IMG_INIT_JPG | IMG_INIT_PNG;
    int initted = IMG_Init(flags);
    checkErrorSdlImg((initted & flags) != flags, "Unable to initialize SDL_image");

    SDL_Rect dest_rect;
    dest_rect.x = 50; dest_rect.y = 50;
    dest_rect.w = 337; dest_rect.h = 210;

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, m_texture.get(), NULL, &dest_rect);
    SDL_RenderPresent(renderer);

    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    cout << "SDL_GetWindowFlags " << endl;
    if (SDL_GetWindowFlags(m_window.get()) | SDL_WINDOW_FULLSCREEN){
        cout << "   SDL_WINDOW_FULLSCREEN" << endl;
    }

    // SDL_GetDesktopDisplayMode
    auto width = DM.w;
    auto height = DM.h;

    std::cout << "width = " << width << std::endl;
    std::cout << "height = " << height << std::endl;
}

TheDisplayer::~TheDisplayer(){
}


double TheDisplayer::getLatestAverageFromQueue(){
    double level = 0.;
    double sum = 0.;
    queue< double > q;
    const int numberOfSampleToAverage = 5;
    //cout << "size_approx before = " << m_lockFreeQueue->size_approx() << endl;

    // we can obtain a sum < 0 if m_lockFreeQueue is not empty here
    while (m_lockFreeQueue->try_dequeue(level)){
        sum += level;
        q.push(level);
        if (q.size() > numberOfSampleToAverage){
            sum -= q.front();
            q.pop();
        }
    }

    //cout << "size_approx after  = " << m_lockFreeQueue->size_approx() << endl;
    level = ((double)sum/(double)numberOfSampleToAverage);
    return level;
}

void TheDisplayer::readAndDisplay(){
    SDL_Delay(2000);
    SDL_Rect contour;
    contour.x = 350; contour.y = 50;
    contour.w = 50; contour.h = 500;

    SDL_Renderer *renderer = m_renderer.get();

    while (1){
        double level = getLatestAverageFromQueue();
        level = log(level)*10;
        if (level >= 100){
            level = 100;
        } else if (level < 0){
            level = 0;
        }

        SDL_SetRenderDrawColor(renderer, 0xE9, 0xF0, 0xF2, 100);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0x3F, 0x77, 0x8A, 100);
        SDL_RenderDrawRect(renderer, &contour);

        SDL_Rect jauge;
        int h = (int)((double)(contour.h*level)/100);
        jauge.x = contour.x; jauge.y = contour.y + contour.h - h;
        jauge.w = 50; jauge.h = h;

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x07, 0x8A, 100);
        SDL_RenderFillRect(renderer, &jauge);

        SDL_RenderPresent(renderer);

        SDL_Delay(30);
    }

}


