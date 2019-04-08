#include <SDL2/SDL.h>

#include "defines.h"
#include "version.h"
//#include "deb-watchtogether.h"
#include "utils.h"

#define GenColor(R,G,B,A) ((R << 16) | (G << 8) | (B) | (A << 24))

#include "deb-watchtogether-v2.h"
#include "watchtogether.c"

global bool32 GlobalRunning;

SDL_Surface* Deb_ResizePixelBuffer(SDL_Window *window)
{
    int width, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    
    // NOTE(Val): Using 0, 0, 0, 0 
    // NOTE(Val): This causes the pixel format to be RRGGBBAA
    return SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0); 
}

int main(int argv, char** argc)
{
    int ret = 0;
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow(WT_WINDOW_TITLE, 100, 100, 640, 480, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        SDL_Quit();
    }
    
    SDL_Surface *surface = Deb_ResizePixelBuffer(window);
    
    game_data data = {};
    
    GlobalRunning = 1;
    while(GlobalRunning)
    {
        // process events
        
        SDL_Event event = {};
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                {
                    GlobalRunning = 0;
                } break;
                case SDL_WINDOWEVENT:
                {
                    switch(event.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        {
                            SDL_FreeSurface(surface);
                            surface = Deb_ResizePixelBuffer(window);
                        } break;
                        default:
                        {
                            
                        } break;
                    }
                } break;
                case SDL_KEYDOWN:
                {
                    // NOTE(Val): Keypress events here
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_SPACE:
                        {
                            gradient_index = (gradient_index + 1) % ArrayCount(gradient);
                        } break;
                        default:
                        {
                            
                        } break;
                    }
                } break;
                default: 
                {
                    
                } break;
            }
        }
        
        // render
        
        // write audio
        
        data.Pixels.buffer = surface->pixels;
        data.Pixels.width = surface->w;
        data.Pixels.height = surface->h;
        
        Processing(&data);
        
        // display
        SDL_Surface *win_surface = SDL_GetWindowSurface(window);
        SDL_BlitSurface(surface, NULL, win_surface, NULL);
        SDL_UpdateWindowSurface(window);
    }
    
    SDL_FreeSurface(surface);
    
    SDL_Quit();
    return 0;
}