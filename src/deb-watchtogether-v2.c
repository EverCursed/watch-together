#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <libavcodec/avcodec.h>


#include "defines.h"
#include "wt_version.h"
#include "utils.h"

#include "deb-watchtogether-v2.h"
#include "watchtogether.c"

/* TODO(Val): A list of things that still must be done 
 --- I guess add ffmpeg stuff here
 --- Provide TCP/UDP support
 --- Touchscreen support
 --- Rewrite audio pushing so that I can push arbitrary 
 ---     lengths of audio into queue
 */

global bool32 GlobalRunning;
global bool32 AudioID;
global float ms_target = 33.0f;
global SDL_Renderer *renderer = NULL;
global SDL_Texture *texture;
global SDL_Texture *background_texture;
global SDL_Texture *ui_texture;

// TODO(Val): remove this, just temporary
static void
set_FPS(float value)
{
    ms_target = value;
}

static void
blit_frame(pixel_buffer buffer)
{
    SDL_Surface *image = SDL_CreateRGBSurfaceWithFormatFrom(
        buffer.buffer,
        buffer.width,
        buffer.height,
        24,
        buffer.width*3,
        SDL_PIXELFORMAT_RGB24
        );
    
    if(!image)
        printf("SDL_CreateRGBSurfaceWithFormatFrom failed!");
    
    if(background_texture)
    {
        SDL_DestroyTexture(background_texture);
        background_texture = NULL;
    }
    background_texture = SDL_CreateTextureFromSurface(
        renderer,
        image
        );
    if(!background_texture)
    {
        printf("Error: %s\n", SDL_GetError()); SDL_ClearError();
    }
    
    SDL_FreeSurface(image);
    free(buffer.buffer);
}

// TODO(Val): Remove this function and use ffmpeg to get sound data
sound_sample load_WAV(const char* filename)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* data_old = malloc(fsize);
    void* data = data_old;
    fread(data , 1, fsize, f);
    fclose(f);
    
    sound_sample descriptor = {};
    sound_sample empty = {};
    
    char riff[] = "RIFF";
    char wave[] = "WAVE";
    char fmt[] = "fmt ";
    char dat[] = "data";
    
    for(uint32 i = 0; i < ArrayCount(riff)-1; i++)
    {
        if(riff[i] != *((char *)data++))
        {
            printf("Not RIFF.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    //uint32 chunk_size = *((uint32 *)data);
    data += 4;
    
    for(uint32 i = 0; i < ArrayCount(wave)-1; i++)
    {
        if(wave[i] != *((char *)data++))
        {
            printf("Not WAVE.\n");
            
            free(data_old);
            
            return empty;
        }
    }
    
    for(uint32 i = 0; i < ArrayCount(fmt)-1; i++)
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
    
    //uint16 AudioFormat = *((uint16 *)data);
    data += 2;
    
    uint16 NumChannels = *((uint16 *)data);
    data += 2;
    
    uint32 SampleRate = *((uint32 *)data);
    data += 4;
    
    //uint32 ByteRate = *((uint32 *)data);
    data += 4;
    
    //uint16 BlockAlign = *((uint16 *)data);
    data += 2;
    
    uint16 BitsPerSample = *((uint16 *)data);
    data += 2;
    
    data = saved_data + Subchunk1Size;
    
    for(uint32 i = 0; i < ArrayCount(dat)-1; i++)
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

/*
static SDL_Texture* 
Deb_ResizePixelBuffer(SDL_Window *window, SDL_Renderer *renderer)
{
SDL_GetWindowSize(window, &width, &height);
return SDL_CreateTexture(renderer,
SDL_PIXELFORMAT_RGB24, // TODO(Val): confirm this
SDL_TEXTUREACCESS_STREAMING,
,
height,
); 
}
*/
static void
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
        uint32 len = (BytesToHaveQueued - CurrentQueuedAudioSize);
        
        // NOTE(Val): This is split into two parts because 
        // it currently loops the audio non-stop
        if(SoundSample->CurrentIndex + len <= SoundSample->Size)
        {
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

static void
PlatformEnqueueAudio(sound_sample *SoundSample)
{
    EnqueueAudio(AudioID, SoundSample);
}

static void 
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

int main()
{
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) != 0){
        return 1;
    }
    
    SDL_Window *window = NULL;
    SDL_CreateWindowAndRenderer(/*WT_WINDOW_TITLE, */1280,
                                720,
                                SDL_WINDOW_BORDERLESS,
                                &window,
                                &renderer);
    
    if(window == NULL || renderer == NULL)
        SDL_Quit();
    
    SDL_SetWindowTitle(window, WT_WINDOW_TITLE);
    SDL_ShowWindow(window);
    
    sound_sample SoundSample = load_WAV("data/audio_test/audio.wav");
    SDL_AudioSpec AudioSpec = {};
    InitializeAudioDevice(SoundSample,
                          &AudioID,
                          &AudioSpec);
    
    av_register_all();
    
    if(video_open("data/video2.mp4"))
        return -1;
    
    SDL_PauseAudioDevice(AudioID, 0); /* start audio playing. */
    
    //texture = Deb_ResizePixelBuffer(window, renderer);
    
    program_data data = {};
    
    struct timespec TimeStart, TimeEnd;
    
    
    GlobalRunning = 1;
    while(GlobalRunning)
    {
        // NOTE(Val): Start timer
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &TimeStart);
        
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
                            //SDL_FreeSurface(surface);
                            //surface = Deb_ResizePixelBuffer(window);
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
        
        
        // render
        
        // write audio
        // EnqueueAudio(AudioID, &SoundSample);
        
        data.Pixels.buffer = NULL; //surface->pixels;
        data.Pixels.width = 0; //surface->w;
        data.Pixels.height = 0; //surface->h;
        
        data.SoundSample = &SoundSample;
        Processing(&data);
        
        blit_frame(data.Pixels);
        
        SDL_SetTextureBlendMode(background_texture, SDL_BLENDMODE_NONE);
        SDL_SetTextureBlendMode(ui_texture, SDL_BLENDMODE_BLEND);
        
        
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);
        
        SDL_RenderPresent(renderer);
        
        //SDL_Surface *win_surface = SDL_GetWindowSurface(window);
        //SDL_UpdateWindowSurface(window);
        // display
        //SDL_BlitSurface(surface, NULL, win_surface, NULL);
        
        // NOTE(Val): Wait here until the end of this frame's time
        // slot to begin next frame processing.
        // TODO(Val): At best, this can be said to be the absolute
        // fucking worst way to do this. This will need to be redone 
        // since only updating the video frame at a certain 
        // framerate is necessary. Redrawing the buffer with
        // appropriate scaling will still need to happen without
        // framerate in consideration
        // Maybe do this in a separate thread? Just sleep until
        // it's time to flip buffers and then sleep again?
        struct timespec TimeDifference = {};
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &TimeEnd);
        TimeDifference.tv_nsec = TimeEnd.tv_nsec - TimeStart.tv_nsec;
        TimeDifference.tv_sec = TimeEnd.tv_sec - TimeStart.tv_sec;
        
        struct timespec sleep_duration;
        sleep_duration.tv_nsec =
            (uint64)(ms_target * 1000000.0f) -
            TimeDifference.tv_nsec;
        sleep_duration.tv_sec =
            (uint64)(ms_target / 1000.0f) -
            TimeDifference.tv_sec +
            (sleep_duration.tv_nsec/1000000000);
        if(sleep_duration.tv_nsec >= 1000000000)
        {
            sleep_duration.tv_sec -= (uint64)sleep_duration.tv_nsec/1000000000.0f;
            sleep_duration.tv_nsec = fmod(sleep_duration.tv_nsec, 1000000000.0f);
        }
        //printf("sec: %ld\tnsec: %ld\n", sleep_duration.tv_sec, sleep_duration.tv_nsec);
        // sleep
        nanosleep(&sleep_duration, NULL);
    }
    
    video_close();
    
    free(SoundSample.Data);
    
    SDL_DestroyTexture(texture);
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(ui_texture);
    
    SDL_CloseAudioDevice(AudioID);
    
    SDL_Quit();
    return 0;
}

