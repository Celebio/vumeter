#include "thedisplayer.hpp"

#include <SDL_image.h>
#include <iostream>
#include <queue>

using namespace std;



TheDisplayer::TheDisplayer(RWQueue *lockFreeQueue) :
    m_lockFreeQueue(lockFreeQueue)
{
    check_error_sdl(SDL_Init(SDL_INIT_VIDEO) != 0, "Unable to initialize SDL");
    m_window = SDL_CreateWindow("Sebastien", 0, 0,
                                          800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    check_error_sdl(m_window == nullptr, "Unable to create m_window");

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    check_error_sdl(m_renderer == nullptr, "Unable to create a m_renderer");

    SDL_SetRenderDrawColor(m_renderer, 100, 149, 237, 255);

    int flags=IMG_INIT_JPG | IMG_INIT_PNG;
    int initted = IMG_Init(flags);
    check_error_sdl_img((initted & flags) != flags, "Unable to initialize SDL_image");

    m_texture = load_texture("img_test.png", m_renderer);

    SDL_Rect dest_rect;
    dest_rect.x = 50; dest_rect.y = 50;
    dest_rect.w = 337; dest_rect.h = 210;

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, NULL, &dest_rect);
    SDL_RenderPresent(m_renderer);


    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    cout << "SDL_GetWindowFlags " << endl;
    if (SDL_GetWindowFlags(m_window) | SDL_WINDOW_FULLSCREEN){
        cout << "   SDL_WINDOW_FULLSCREEN" << endl;
    }

    // SDL_GetDesktopDisplayMode
    auto width = DM.w;
    auto height = DM.h;

    std::cout << "width = " << width << std::endl;
    std::cout << "height = " << height << std::endl;




}

TheDisplayer::~TheDisplayer(){
    SDL_DestroyTexture(m_texture);
    IMG_Quit();

    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
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

    while (1){
        double level = getLatestAverageFromQueue();
        level = log(level)*10;
        if (level >= 100){
            level = 100;
        } else if (level < 0){
            level = 0;
        }

        SDL_SetRenderDrawColor(m_renderer, 0xE9, 0xF0, 0xF2, 100);
        SDL_RenderClear(m_renderer);

        SDL_SetRenderDrawColor(m_renderer, 0x3F, 0x77, 0x8A, 100);
        SDL_RenderDrawRect(m_renderer, &contour);

        SDL_Rect jauge;
        int h = (int)((double)(contour.h*level)/100);
        jauge.x = contour.x; jauge.y = contour.y + contour.h - h;
        jauge.w = 50; jauge.h = h;

        SDL_SetRenderDrawColor(m_renderer, 0xFF, 0x07, 0x8A, 100);
        SDL_RenderFillRect(m_renderer, &jauge);

        SDL_RenderPresent(m_renderer);

        SDL_Delay(30);
    }

}

void TheDisplayer::check_error_sdl(bool check, const char* message) {
    if (check) {
        std::cout << message << " " << SDL_GetError() << std::endl;
        SDL_Quit();
        std::exit(-1);
    }
}



void TheDisplayer::check_error_sdl_img(bool check, const char* message) {
    if (check) {
        std::cout << message << " " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        std::exit(-1);
    }
}

SDL_Texture* TheDisplayer::load_texture(const char* fname, SDL_Renderer *renderer) {
    SDL_Surface *image = IMG_Load(fname);
    check_error_sdl_img(image == nullptr, "Unable to load image");

    SDL_Texture *img_texture = SDL_CreateTextureFromSurface(renderer, image);
    check_error_sdl_img(img_texture == nullptr, "Unable to create a texture from the image");
    SDL_FreeSurface(image);
    return img_texture;
}
