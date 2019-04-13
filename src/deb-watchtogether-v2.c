#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>

#include "defines.h"
#include "version.h"
#include "utils.h"

#include "deb-watchtogether-v2.h"
#include "watchtogether.c"

/* TODO(Val): A list of things that still must be done 
 --- Provide TCP/UDP support
 --- Touchscreen support
 --- Rewrite audio pushing so that I can push arbitrary 
 ---     lengths of audio into queue
 */

global bool32 GlobalRunning;
global bool32 AudioID;

internal int
load_ffmpeg()
{
    
}

sound_sample load_WAV(const char* filename)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* data_old = malloc(fsize);
    void* data = data_old;
    size_t data_read = fread(data , 1, fsize, f);
    fclose(f);
    
    sound_sample descriptor = {};
    sound_sample empty = {};
    
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
    
    descriptor.Data = final_data;
    descriptor.Size = Subchunk2Size;
    descriptor.Frequency = SampleRate;
    descriptor.Channels = NumChannels;
    descriptor.BitsPerSample = BitsPerSample;
    
    return descriptor;
}

internal SDL_Surface* 
Deb_ResizePixelBuffer(SDL_Window *window)
{
    int width, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    
    // NOTE(Val): Using 0, 0, 0, 0 
    // NOTE(Val): This causes the pixel format to be RRGGBBAA
    return SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0); 
}

internal void
EnqueueAudio(SDL_AudioDeviceID AudioID,
             sound_sample *SoundSample)
{
    uint32 CurrentQueuedAudioSize = 
        SDL_GetQueuedAudioSize(AudioID);
    
    // NOTE(Val): Queue up 0.5secs of audio.
    uint32 BytesPerSec =
        SoundSample->Frequency * \
        (SoundSample->BitsPerSample/8) * \
        SoundSample->Channels;
    uint32 BytesToHaveQueued = BytesPerSec / 2;
    
    if(CurrentQueuedAudioSize < BytesToHaveQueued)
    {
        printf("%d\n", BytesToHaveQueued);
        printf("%d\n", CurrentQueuedAudioSize);
        uint32 len = (BytesToHaveQueued - CurrentQueuedAudioSize);
        
        // NOTE(Val): This is split into two parts because 
        // it currently loops the audio non-stop
        if(SoundSample->CurrentIndex + len <= SoundSample->Size)
        {
            printf("CurrentIndex = %d\n", SoundSample->CurrentIndex);
            SDL_QueueAudio(
                AudioID,
                (SoundSample->Data + SoundSample->CurrentIndex),
                len);
            
            SoundSample->CurrentIndex += len;
        }
        else
        {
            int Part1Size = SoundSample->Size - SoundSample->CurrentIndex;
            int Part2Size = len - Part1Size;
            
            printf("Part1Size\t= %d\n"
                   "Part2Size\t= %d\n"
                   "Size\t\t= %d\n"
                   "CurrentIndex\t= %d\n"
                   "Buffer\t\t= %p\n"
                   "Buffer1\t= %p\n",
                   Part1Size,
                   Part2Size,
                   SoundSample->Size,
                   SoundSample->CurrentIndex,
                   SoundSample->Data,
                   (SoundSample->Data + SoundSample->CurrentIndex)
                   );
            SDL_QueueAudio(
                AudioID,
                (SoundSample->Data + SoundSample->CurrentIndex),
                Part1Size);
            SDL_QueueAudio(
                AudioID,
                SoundSample->Data,
                Part2Size);
            
            SoundSample->CurrentIndex = Part2Size;
        }
        
    }
}

internal void
PlatformEnqueueAudio(sound_sample *SoundSample)
{
    EnqueueAudio(AudioID, SoundSample);
}

internal void 
InitializeAudioDevice(sound_sample SoundSample,
                      SDL_AudioDeviceID *AudioID,
                      SDL_AudioSpec *AudioSpec)
{
    SDL_AudioSpec DesiredAudioSpec = {};
    
    DesiredAudioSpec.freq = SoundSample.Frequency;
    // NOTE(Val): Should depend on source audio
    if(SoundSample.BitsPerSample == 8)
        DesiredAudioSpec.format = AUDIO_U8;
    else if(SoundSample.BitsPerSample == 16)
        DesiredAudioSpec.format = AUDIO_U16;
    else if(SoundSample.BitsPerSample == 32)
        DesiredAudioSpec.format = AUDIO_S32;
    
    DesiredAudioSpec.channels = SoundSample.Channels;
    
    // NOTE(Val): Sending a number of samples to the audio device
    // prevents you from immediately closing the app until the
    // entire buffer is played. Therefore this is small.
    // TODO(Val): Test to see what the smallest value we can set 
    // to and how that would affect performance.
    DesiredAudioSpec.samples = 1024;
    DesiredAudioSpec.callback = NULL;
    DesiredAudioSpec.userdata = NULL;
    
    // TODO(Val): See if there's a different way to do this 
    // other than closing and reopening the same sound device
    if(*AudioID != 0)
        SDL_CloseAudioDevice(*AudioID);
    
    *AudioID = 
        SDL_OpenAudioDevice(NULL,
                            0,
                            &DesiredAudioSpec,
                            AudioSpec,
                            SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    //printf("%s\n", SDL_GetError());
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
    
    sound_sample SoundSample = load_WAV("data/audio_test/audio.wav");
    SDL_AudioSpec AudioSpec = {};
    InitializeAudioDevice(SoundSample,
                          &AudioID,
                          &AudioSpec);
    
    SDL_PauseAudioDevice(AudioID, 0); /* start audio playing. */
    
    
    SDL_Surface *surface = Deb_ResizePixelBuffer(window);
    
    program_data data = {};
    
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
                            GlobalRunning = 0;
                        } break;
                        case SDLK_RETURN:
                        {
                            SoundSample = load_WAV("data/audio_test/audio2.wav");
                            InitializeAudioDevice(SoundSample,
                                                  &AudioID,
                                                  &AudioSpec);
                            SDL_PauseAudioDevice(AudioID, 0);
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
            
            if(!GlobalRunning)
                break;
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
        // EnqueueAudio(AudioID, &SoundSample);
        
        data.Pixels.buffer = surface->pixels;
        data.Pixels.width = surface->w;
        data.Pixels.height = surface->h;
        
        data.SoundSample = &SoundSample;
        Processing(&data);
        
        // display
        SDL_Surface *win_surface = SDL_GetWindowSurface(window);
        SDL_BlitSurface(surface, NULL, win_surface, NULL);
        SDL_UpdateWindowSurface(window);
    }
    
    free(SoundSample.Data);
    
    SDL_CloseAudioDevice(AudioID);
    SDL_FreeSurface(surface);
    
    SDL_Quit();
    return 0;
}
