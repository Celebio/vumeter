#include "displayer.hpp"

#include <SDL_image.h>
#include <iostream>
#include <queue>
#include <system_error>

using namespace std;

SDLResource* SDLResource::m_instance;

SDLResource::SDLResource(){
    SDL_Init(SDL_INIT_VIDEO);
}
SDLResource::~SDLResource(){
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


Displayer::Displayer(RWQueue *lockFreeQueue,
                     RWVectorQueue *lockFreeVectorQueue) :
    m_lockFreeQueue(lockFreeQueue),
    m_lockFreeVectorQueue(lockFreeVectorQueue),
    m_sdlResource(SDLResource::getInstance()),
    m_window(makeResource(SDL_CreateWindow, SDL_DestroyWindow, "Sebastien", 0, 0, 1400, 700, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)),
    m_renderer(makeResource(SDL_CreateRenderer, SDL_DestroyRenderer, m_window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)),
    m_texture(makeResource(loadTexture, SDL_DestroyTexture, "img_test.png", m_renderer.get())),
    m_lastFrequencyAmplitudes({})
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

Displayer::~Displayer(){
}

void Displayer::fetchLatestFrequencyAmplitudes() {
    int ctr = 0;
    while (m_lockFreeVectorQueue->try_dequeue(m_lastFrequencyAmplitudes)
           && ((ctr++) < 10));
    // cout << "Dropped : " << ctr << endl;
}

void Displayer::fetchLatestAverageFromQueue(){
    double level = 0.;
    double sum = 0.;
    queue< double > q;
    const int numberOfSampleToAverage = 1;
    // //cout << "size_approx before = " << m_lockFreeQueue->size_approx() << endl;

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

    if (level != 0){
        m_level = log(level)*10;
        if (m_level >= 100){
            m_level = 100;
        } else if (m_level < 0){
            m_level = 0;
        }
        // cout << "m_level = " << m_level << endl;
    }
}

void Displayer::readAndDisplay(){
    SDL_Delay(2000);
    SDL_Rect contour;
    contour.x = 350; contour.y = 50;
    contour.w = 50; contour.h = 300;

    SDL_Renderer *renderer = m_renderer.get();

    while (1){
        fetchLatestAverageFromQueue();
        fetchLatestFrequencyAmplitudes();

        SDL_SetRenderDrawColor(renderer, 0xE9, 0xF0, 0xF2, 100);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0x3F, 0x77, 0x8A, 100);
        SDL_RenderDrawRect(renderer, &contour);

        SDL_Rect jauge;
        int h = (int)((double)(contour.h*m_level)/100);
        jauge.x = contour.x; jauge.y = contour.y + contour.h - h;
        jauge.w = 50; jauge.h = h;

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x07, 0x8A, 100);
        SDL_RenderFillRect(renderer, &jauge);

        {
            const int numberOfSticks = m_lastFrequencyAmplitudes.size();
            const int stickWidth = 4;
            const int stickMargin = 1;
            int curX = 10;
            int curY = 400;

            SDL_Rect contour;
            SDL_Rect jauge;
            for (int i=0; i<numberOfSticks/2; i++){
                contour.x = curX; contour.y = curY;
                contour.w = stickWidth; contour.h = 150;

                SDL_SetRenderDrawColor(renderer, 0xf7, 0x85, 0xc1, 255);
                SDL_RenderDrawRect(renderer, &contour);


                double level = m_lastFrequencyAmplitudes[i]*30;
                if (level > 100) level = 100;
                if (level < 0) level = 0;
                int h = (int)((double)(contour.h*level)/100);
                jauge.x = contour.x; jauge.y = contour.y + contour.h - h;
                jauge.w = stickWidth; jauge.h = h;

                SDL_SetRenderDrawColor(renderer, 0xFF, 0x07, 0x8A, 100);
                SDL_RenderFillRect(renderer, &jauge);

                curX += (stickWidth+stickMargin);
            }
        }



        SDL_RenderPresent(renderer);

        SDL_Delay(20);
    }

}


