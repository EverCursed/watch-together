#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "version.h"
//#include "deb-watchtogether.h"
#include "utils.h"

#define GenColor(R,G,B,A) ((R << 16) | (G << 8) | (B) | (A << 24))

#include "deb-watchtogether-v2.h"
#include "watchtogether.c"

global bool32 GlobalRunning;

typedef struct {
    uint32 Frequency;
    uint16 Channels;
    uint16 BitsPerSample;
    uint32 Size;
    void *data;
    uint32 CurrentIndex;
} WAV_descriptor;


WAV_descriptor load_WAV(const char* filename)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* data_old = malloc(fsize);
    void* data = data_old;
    fread(data , 1, fsize, f);
    fclose(f);
    
    WAV_descriptor descriptor = {};
    WAV_descriptor empty = {};
    
    char riff[] = "RIFF";
    char wave[] = "WAVE";
    char fmt[] = "fmt ";
    char dat[] = "data";
    
    for(int i = 0; i < ArrayCount(riff)-1; i++)
    {
        if(riff[i] != *((char *)data++))
        {
            printf("Not RIFF.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    uint32 chunk_size = *((uint32 *)data);
    data += 4;
    
    for(int i = 0; i < ArrayCount(wave)-1; i++)
    {
        if(wave[i] != *((char *)data++))
        {
            printf("Not WAVE.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    for(int i = 0; i < ArrayCount(fmt)-1; i++)
    {
        if(fmt[i] != *((char *)data++))
        {
            printf("Not fmt.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    
    
    uint32 Subchunk1Size = *((uint32 *)data);
    data += 4;
    void* saved_data = data;
    
    uint16 AudioFormat = *((uint16 *)data);
    data += 2;
    
    uint16 NumChannels = *((uint16 *)data);
    data += 2;
    
    uint32 SampleRate = *((uint32 *)data);
    data += 4;
    
    uint32 ByteRate = *((uint32 *)data);
    data += 4;
    
    uint16 BlockAlign = *((uint16 *)data);
    data += 2;
    
    uint16 BitsPerSample = *((uint16 *)data);
    data += 2;
    
    data = saved_data + Subchunk1Size;
    
    for(int i = 0; i < ArrayCount(dat)-1; i++)
    {
        if(dat[i] != *((char *)data++))
        {
            printf("Not data.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    uint32 Subchunk2Size = *((uint32 *)data);
    data += 4;
    
    void* final_data = malloc(Subchunk2Size);
    memcpy(final_data, data, Subchunk2Size);
    free(data_old);
    
    descriptor.Frequency = SampleRate;
    descriptor.Channels = NumChannels;
    descriptor.BitsPerSample = BitsPerSample;
    descriptor.data = final_data;
    descriptor.Size = Subchunk2Size;
    
    return descriptor;
}

SDL_Surface* Deb_ResizePixelBuffer(SDL_Window *window)
{
    int width, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    
    // NOTE(Val): Using 0, 0, 0, 0 
    // NOTE(Val): This causes the pixel format to be RRGGBBAA
    return SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0); 
}



void AudioCallback(void*  userdata,
                   Uint8* stream,
                   int    len)
{
    // TODO(Val): Need this to loop until one buffer 
    // and continue filling with another buffer;
    WAV_descriptor *descriptor = (WAV_descriptor *)userdata;
    uint8* buffer = descriptor->data;
    
    uint32 size = descriptor->Size;
    uint32 index = descriptor->CurrentIndex;
    
    if(size - index >= len)
    {
        memcpy(stream, (buffer+index), len);
        descriptor->CurrentIndex =
            (descriptor->CurrentIndex + len) % descriptor->Size;
    }
    else
    {
        uint32 bytes = size - index;
        memcpy(stream, (buffer+index), bytes);
        memcpy((stream+bytes), buffer, len-bytes);
        descriptor->CurrentIndex = 
            (descriptor->CurrentIndex + len) % descriptor->Size;
    }
    
}

int main(int argv, char** argc)
{
    int ret = 0;
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0){
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow(WT_WINDOW_TITLE, 100, 100, 640, 480, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        SDL_Quit();
    }
    
    WAV_descriptor wav_descriptor = load_WAV("data/audio_test/audio.wav");
    
    SDL_AudioSpec DesiredAudioSpec = {};
    /*
    AudioSpec
    -------------
    freq
    format
    channels
    silence
    samples
    size
    callback
    userdata
    */
    DesiredAudioSpec.freq = wav_descriptor.Frequency;
    // NOTE(Val): Should depend on source audio
    DesiredAudioSpec.format = AUDIO_U16;
    DesiredAudioSpec.channels = wav_descriptor.Channels;
    
    // NOTE(Val): Sending a number of samples to the audio device
    // prevents you from immediately closing the app until the
    // entire buffer is played. 
    DesiredAudioSpec.samples = 4096; //(uint32)(wav_descriptor.Frequency);
    DesiredAudioSpec.callback = (SDL_AudioCallback)AudioCallback;
    DesiredAudioSpec.userdata = &wav_descriptor;
    
    SDL_AudioSpec AudioSpec = {};
    SDL_AudioDeviceID AudioID = 
        SDL_OpenAudioDevice(NULL,
                            0,
                            &DesiredAudioSpec,
                            &AudioSpec,
                            SDL_AUDIO_ALLOW_ANY_CHANGE);
    printf("Freq\t\t%d\n"
           "Channels\t%d\n"
           "Samples\t\t%d\n", 
           AudioSpec.freq,
           AudioSpec.channels,
           AudioSpec.samples
           );
    
    SDL_PauseAudioDevice(AudioID, 0); /* start audio playing. */
    printf("%s\n", SDL_GetError());
    
    
    SDL_Surface *surface = Deb_ResizePixelBuffer(window);
    
    game_data data = {};
    
    GlobalRunning = 1;
    while(GlobalRunning)
    {
        // NOTE(Val): Process events
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
                        case SDLK_ESCAPE:
                        {
                            SDL_UnlockAudioDevice(AudioID);
                            GlobalRunning = 0;
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
        
        if(!GlobalRunning)
            break;
        
        // NOTE(Val): Process mouse
        
        data.Mouse.old_x = data.Mouse.x;
        data.Mouse.old_y = data.Mouse.y;
        data.Mouse.left_button_was_pressed = data.Mouse.left_button_is_pressed;
        data.Mouse.right_button_was_pressed = data.Mouse.right_button_is_pressed;
        
        uint32 mouse_state = SDL_GetMouseState(&data.Mouse.x, &data.Mouse.y);
        if(mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT))
            data.Mouse.left_button_is_pressed = 1;
        else
            data.Mouse.left_button_is_pressed = 0;
        
        if(mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT))
            data.Mouse.right_button_is_pressed = 1;
        else
            data.Mouse.right_button_is_pressed = 0;
        
        // printf("Mouse:\tx: %d\n\t\ty:%d\n", data.Mouse.x, data.Mouse.y);
        
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
    
    free(wav_descriptor.data);
    
    SDL_PauseAudioDevice(AudioID, 1);
    SDL_CloseAudioDevice(AudioID);
    SDL_FreeSurface(surface);
    
    SDL_Quit();
    return 0;
}



