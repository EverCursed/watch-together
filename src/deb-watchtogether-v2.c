#include <SDL2/SDL.h>

#include "defines.h"
#include "version.h"
#include "deb-watchtogether.h"
#include "utils.h"

global bool32 GlobalRunning;


struct pixel {
    union{
        struct {
            uint8 blue;
            uint8 green;
            uint8 red;
            uint8 alpha;
        };
        uint32 color;
    };
};


void draw_gradient(SDL_Surface *surface)
{
    uint32 gradient[] = {
        0x00FF0000,
        0x00FFFF00,
        0x0000FF00,
        0x0000FFFF,
        0x000000FF,
        //0x00FF00FF,
        //0x00FF0000,
    };
    SDL_LockSurface(surface);
    for(int x = 0; x < surface->w; x++)
    {
        real32 grad = (real32)surface->w / (ArrayCount(gradient)-1);
        uint32 color = (uint32)(x / grad);
        real32 pct1 = (real32)(x - color*grad ) / (real32)grad; 
        real32 pct2 = 1.0f - pct1; 
        for(int y = 0; y < surface->h; y++)
        {
            struct pixel *p = (struct pixel *)((uint32 *)surface->pixels + surface->w*y + x);
            
            struct pixel color1 = {};
            color1.color = (uint32)gradient[color+1]; 
            color1.red *= pct1;
            color1.green *= pct1;
            color1.blue *= pct1;
            
            color1.red *= ((real32)(surface->h-y) / (real32)surface->h);
            color1.green *= ((real32)(surface->h-y) / (real32)surface->h);
            color1.blue *= ((real32)(surface->h-y) / (real32)surface->h);
            
            struct pixel color2 = {};
            color2.color = (real32)gradient[color]; 
            color2.red *= pct2;
            color2.green *= pct2;
            color2.blue *= pct2;
            
            color2.red *= ((real32)(surface->h-y) / (real32)surface->h);
            color2.green *= ((real32)(surface->h-y) / (real32)surface->h);
            color2.blue *= ((real32)(surface->h-y) / (real32)surface->h);
            
            
            p->color = (color1.color + color2.color);
            //((uint32)( + (real32)gradient[color+1]*pct2)) * (1.0f - (real32)y/(real32)surface->h);
            
        }
    }
    SDL_UnlockSurface(surface);
}

int main(int argv, char** argc)
{
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
		return 1;
	}
    
    SDL_Window *window = SDL_CreateWindow(WT_WINDOW_TITLE, 100, 100, 640, 480, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        SDL_Quit();
    }
    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    
    SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    
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
                            
                            int width, height;
                            SDL_GetWindowSize(window, &width, &height);
                            
                            surface = SDL_CreateRGBSurface(0, event.window.data1, event.window.data2, 32, 0, 0, 0, 0);
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
        draw_gradient(surface);
        /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
       as expected by OpenGL for textures */
        
        
        // write audio
        
        
        
        // display
        SDL_Surface *win_surface = SDL_GetWindowSurface(window);
        
        SDL_BlitSurface(surface, NULL, win_surface, NULL);
        SDL_UpdateWindowSurface(window);
    }
    
    SDL_FreeSurface(surface);
    
    SDL_Quit();
    return 0;
}