/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <libavutil/frame.h>

#include "watchtogether.h"
#include "utils/timing.h"
#include "utils.h"
#include "logging.h"

#include "config.h"

#if 0
#ifdef _WIN32
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
#endif
#endif

//#include "defines.h"
//#include "wt_version.h"
//#include "utils.h"
//#include "kbkeys.h"

//#include "SDL_platform.h"
//#include "watchtogether.c"

/* TODO(Val): A list of things that still must be done 
 --- Touchscreen support
 */

global SDL_AudioDeviceID AudioID = -1;
global bool32 audio_initialized = 0;
global SDL_Window *window = NULL;
global SDL_Renderer *renderer = NULL;
//global SDL_Texture *background_texture;
//global SDL_Texture *ui_texture;
global SDL_Texture *final_texture = NULL;
global SDL_Texture *video_texture = NULL;
global SDL_Texture *ui_background_texture = NULL;
global SDL_Texture *ui_texture = NULL;

global SDL_Rect video_output_rect = {};

global program_data *pdata;

#define TESTING_FILE "data/video_test/video.mp4"

internal int32
PlatformQueueAudio(AVFrame *frame)
{
    //output_audio *audio = &pdata->audio;
    int32 size = frame->nb_samples * frame->channels * av_get_bytes_per_sample(frame->format);
    
    int ret = SDL_QueueAudio(AudioID,
                             frame->data[0],
                             size);
    
    if(ret < 0)
    {
        wlog(LOG_ERROR, "Failed to queue audio samples");
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

internal int32
PlatformInitAudio(open_file_info *file)
{
    output_audio *audio = &pdata->audio;
    
    SDL_AudioSpec DesiredAudioSpec = {};
    SDL_AudioSpec ReceivedAudioSpec = {};
    int32 bytes_per_sample = file->bytes_per_sample;
    
    DesiredAudioSpec.freq = file->sample_rate;
    
    // NOTE(Val): Should depend on source audio
    if(file->sample_format == SAMPLE_U8)
    {
        DesiredAudioSpec.format = AUDIO_U8;
    }
    else if(file->sample_format == SAMPLE_S16)
    {
        DesiredAudioSpec.format = AUDIO_S16;
    }
    else if(file->sample_format == SAMPLE_S32)
    {
        DesiredAudioSpec.format = AUDIO_S32;
    }
    else if(file->sample_format == SAMPLE_FLOAT)
    {
        DesiredAudioSpec.format = AUDIO_F32;
    }
    else
    {
        wlog(LOG_ERROR, "The file has an unknown audio format");
        RETURN(UNKNOWN_FORMAT);
    }
    
    DesiredAudioSpec.channels = file->channels;
    
    // NOTE(Val): Sending a number of samples to the audio device
    // prevents you from immediately closing the app until the
    // entire buffer is played. Therefore this is small.
    DesiredAudioSpec.samples = 4096;
    //DesiredAudioSpec.callback = AudioCallback;
    //DesiredAudioSpec.userdata = pdata;
    
    // TODO(Val): See if there's a different way to do this 
    // other than closing and reopening the same sound device
    AudioID = SDL_OpenAudioDevice(NULL,
                                  0,
                                  &DesiredAudioSpec,
                                  &ReceivedAudioSpec,
                                  SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if(AudioID != -1)
    {
        audio_initialized = 1;
        
        audio->sample_rate = ReceivedAudioSpec.freq;
        audio->channels = ReceivedAudioSpec.channels;
        
        // TODO(Val): This doesnt check what audio device params we received.
        audio->bytes_per_sample = bytes_per_sample;
        audio->sample_format = file->sample_format;
        
        pdata->audio.audio_format = ReceivedAudioSpec.format;
        
        //SDL_PauseAudioDevice(AudioID, 0);
    }
    else
    {
        dbg_error("SDL_OpenAudioDevice() failed.\n");
        wlog(LOG_ERROR, "SDL_OpenAudioDevice() failed");
        
        RETURN(UNKNOWN_ERROR);
    }
    //printf("%s\n", SDL_GetError());
    
    RETURN(SUCCESS);
}

internal void
PlatformCloseAudio()
{
    SDL_CloseAudioDevice(AudioID);
}

internal void
PlatformPauseAudio(bool32 b)
{
    SDL_PauseAudioDevice(AudioID, b);
}

/// Platform create thread
internal thread_info
PlatformCreateThread(int32 (*f)(void *), void *data, char* name)
{
    thread_info info = {};
    char* title = name == NULL ? "" : name;
    
    info.thread = SDL_CreateThread(f,
                                   title,
                                   data);
    
    return info;
}

internal void
PlatformWaitThread(thread_info thread, int32 *ret)
{
    SDL_WaitThread(thread.thread, ret);
}

internal cond_info
PlatformCreateConditionVar()
{
    cond_info c = {};
    c.mutex = SDL_CreateMutex();
    c.cond = SDL_CreateCond();
    c.test = 0;
    return c;
}

internal int32
PlatformConditionWait(cond_info *c)
{
    if(SDL_LockMutex(c->mutex))
        RETURN(UNKNOWN_ERROR);
    
    wlog(LOG_TRACE, "Waiting for condition variable");
    while(!c->test && pdata->running)
    {
        SDL_CondWait(c->cond, c->mutex);
    }
    c->test = 0;
    wlog(LOG_TRACE, "Finished waiting");
    SDL_UnlockMutex(c->mutex);
    
    //while(!c->test && pdata->running)
    //{
    //PlatformSleep(0.001);
    //}
    //c->test = 0;
    RETURN(SUCCESS);
}

internal int32
PlatformConditionSignal(cond_info *c)
{
    wlog(LOG_TRACE, "Signaling condition variable");
    
    if(SDL_LockMutex(c->mutex))
        RETURN(UNKNOWN_ERROR);
    
    c->test = 1;
    SDL_CondSignal(c->cond);
    SDL_UnlockMutex(c->mutex);
    
    //c->test = 1;
    RETURN(SUCCESS);
}

internal int32
PlatformConditionDestroy(cond_info *c)
{
    SDL_DestroyMutex(c->mutex);
    SDL_DestroyCond(c->cond);
    
    RETURN(SUCCESS);
}

internal platform_mutex
PlatformCreateMutex()
{
    platform_mutex m = {};
    m.mutex = SDL_CreateMutex();
    if(!m.mutex)
        SDL_PrintError();
    return m;
}

internal int32
PlatformLockMutex(platform_mutex *m)
{
    if(!SDL_LockMutex(m->mutex))
        RETURN(SUCCESS);
    else
    {
        wlog(LOG_ERROR, "Failed to lock mutex");
        RETURN(UNKNOWN_ERROR);
    }
}

internal int32
PlatformUnlockMutex(platform_mutex *m)
{
    if(!SDL_UnlockMutex(m->mutex))
    {
        RETURN(SUCCESS);
    }
    else
    {
        wlog(LOG_ERROR, "Failed to unlock mutex");
        
        RETURN(UNKNOWN_ERROR);
    }
}

internal void
PlatformDestroyMutex(platform_mutex *m)
{
    SDL_DestroyMutex(m->mutex);
}

internal int32
PlatformUpdateVideoFrame(AVFrame *frame)
{
    StartTimer("PlatformUpdateVideoFrame()");
    
    int ret = 0;
    
    StartTimer("SDL_UpdateTexture()");
    
    ret = SDL_UpdateTexture(video_texture,
                            NULL,
                            frame->data[0],
                            frame->linesize[0]);
    SDL_Test(ret);
    EndTimer();
    
    /*
StartTimer("memcpy");
for(int i = 0; i < video->height; i++)
{
memcpy(data + i*pitch, video->video_frame + i*video->pitch, video->pitch);
}
EndTimer();
*/
    
    EndTimer();
    /*
    if(pdata->video.type == VIDEO_YUV)
    {
        int ret = SDL_UpdateYUVTexture(background_texture, NULL,
                                       video->video_frame, video->pitch,
                                       video->video_frame_sup1, video->pitch_sup1,
                                       video->video_frame_sup2, video->pitch_sup2);
                                       
        if(ret < 0)
        {
            dbg_error("SDL_UpdateYUVTexture failed.\n");
            dbg_error("%s\n", SDL_GetError());
            RETURN(UNKNOWN_ERROR);
        }
    }
    else if(pdata->video.type == VIDEO_RGB)
    {
        int ret = SDL_UpdateTexture(background_texture, NULL, video->video_frame, video->pitch);
        //free(video->video_frame);
        
        if(ret < 0)
        {
            dbg_error("SDL_UpdateTexture failed.\n");
            dbg_error("%s\n", SDL_GetError());
            RETURN(UNKNOWN_ERROR);
        }
    }
    else
    {
        dbg_error("Video output frame not initialized?");
        RETURN(UNKNOWN_FORMAT);
    }
    */
    RETURN(SUCCESS);
}

internal int32
PlatformRender()
{
    StartTimer("PlatformUpdateFrame");
    
    int ret = 0;
    
    StartTimer("SDL_RenderClear()");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    ret = SDL_RenderClear(renderer);
    SDL_Test(ret);
    EndTimer();
    
    StartTimer("SDL_RenderCopy(video_texture)");
    if(video_texture)
        ret = SDL_RenderCopy(renderer,
                             video_texture,
                             NULL,
                             &video_output_rect);
    SDL_Test(ret);
    EndTimer();
    
    if(ui_texture)
    {
        SDL_DestroyTexture(ui_texture);
    }
    
    // TODO(Val): Have some kind of flag to check if menu changed
    
    Menu *m = pdata->menu;
    MenuScreen *s;
    if(s = GetTopmostMenuScreen(m))
    {
        i32 width, height;
        SDL_GetRendererOutputSize(renderer, &width, &height);
        
        RenderMenu(m, width, height);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_RenderFillRect(renderer, NULL);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 64);
        SDL_RenderFillRects(renderer, m->rects, m->rects_count);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRects(renderer, m->rects, m->rects_count);
        
        for(int i = 0; i < m->text_count; i++)
        {
            SDL_RenderCopy(renderer,
                           m->textures[i],
                           NULL,
                           &m->text_rects[i]);
        }
        
        custom_free(m->textures);
        custom_free(m->rects);
        custom_free(m->text_rects);
    }
    
    EndTimer();
    RETURN(SUCCESS);
}

internal int32
PlatformFlipBuffers()
{
    wlog(LOG_TRACE, "Flipping pixel buffers");
    
    StartTimer("PlatformFlipBuffers()");
    SDL_RenderPresent(renderer);
    EndTimer();
    
    RETURN(SUCCESS);
}

internal void
PlatformChangeFullscreenState(b32 on)
{
    wlog(LOG_INFO, "Toggling fullscreen");
    
    int ret = 
        SDL_SetWindowFullscreen(window, on * SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    SDL_Test(ret);
}

internal inline
void add_key(input_struct *input,
             uint32 key,
             bool32 shift,
             bool32 ctrl,
             bool32 alt,
             bool32 pressed)
{
    if(input->keyboard.n < MAX_KEYS)
    {
        int n = input->keyboard.n;
        input->keyboard.events[n].key = key;
        input->keyboard.events[n].pressed = pressed;
        input->keyboard.events[n].shift = shift;
        input->keyboard.events[n].ctrl = ctrl;
        input->keyboard.events[n].alt = alt;
        input->keyboard.n++;
    }
}

#ifdef _WIN32
internal int resize_filter(void *userdata,
                           SDL_Event *event)
{
    /*
    if(event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
    {
    dbg_error("SDL_WINDOWEVENT_SIZE_CHANGED filtered.\n");
    
    // NOTE(Val): This happens on any window resize 
    
    //SDL_Rect size;
    //SDL_RenderSetViewport(renderer, NULL);
    //SDL_WindowGet
    
    //int ret = SDL_RenderCopy(renderer, background_texture, NULL, NULL);
    //if(ret)
    //{
    //dbg_error(SDL_GetError());
    //dbg_print("\n");
    //}
    //SDL_RenderCopy(renderer, ui_texture, NULL, NULL);
    
    //SDL_RenderPresent(renderer);
    
    //SDL_SetVideoMode(event->resize.w,event->resize.h,0,SDL_ANYFORMAT | SDL_RESIZABLE);
    //draw();
    return 0;
    }
    else if(event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
    dbg_error("SDL_WINDOWEVENT_RESIZED filtered.\n");
    
    // NOTE(Val): This happens if the window is resized by the user.
    
    // NOTE(Val): This follows SDL_WINDOWEVENT_SIZE_CHANGED in all cases.
    
    
    
    
    return 0;
    }
    */
    return 1; // return 1 so all events are added to queue
}
#endif

internal SDL_Rect
GetVideoOutputRect(open_file_info *file, int screen_width, int screen_height)
{
    SDL_Rect rect = {};
    
    int video_width = file->width;
    int video_height = file->height;
    
    float screen_ratio = (float)screen_width/(float)screen_height;
    float video_ratio = (float)video_width/(float)video_height;
    
    if(screen_ratio < video_ratio)
    {
        // screen wider than video
        
        rect.x = 0;
        rect.y = (screen_height - screen_width/video_ratio)/2;
        rect.w = screen_width;
        rect.h = screen_width/video_ratio;
    }
    else
    {
        // video wider than screen
        
        rect.x = (screen_width - screen_height*video_ratio)/2;
        rect.y = 0;
        rect.w = screen_height*video_ratio;
        rect.h = screen_height;
    }
    
    return rect;
}

internal void
PlatformInitVideo(open_file_info *file)
{
    if(video_texture) SDL_DestroyTexture(video_texture);
    
    video_texture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_BGRA32,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      file->width,
                                      file->height);
    
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    video_output_rect = GetVideoOutputRect(file, w, h);
    
    if(!video_texture) SDL_PrintError();
}

internal int32
PlatformGetInput()
{
    StartTimer("PlatformGetInput()");
    
    input_struct *input = &pdata->input;
    
    SDL_Event event = {0};
    
    StartTimer("Event loop");
    StartTimer("SDL_WaitEventTimeout()");
    while(pdata->running &&
          input->keyboard.n < MAX_KEYS-1 &&
          SDL_PollEvent(&event))
    {
        EndTimer();
        
        switch(event.type)
        {
            case SDL_QUIT:
            {
                wlog(LOG_INFO, "SDL_QUIT message received");
                pdata->running = 0;
                RETURN(SUCCESS);
            } break;
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        wlog(LOG_INFO, "SDL_WINDOWEVENT_RESIZED received");
                        
                        int width = event.window.data1;
                        int height = event.window.data2;
                        
                        video_output_rect = GetVideoOutputRect(&pdata->file, width, height);
                        
                        pdata->client.output_width = width;
                        pdata->client.output_height = height;
                    } break;
                    default:
                    {
                        
                    } break;
                }
            } break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                bool32 shift_down = event.key.keysym.mod & KMOD_SHIFT;
                bool32 ctrl_down = event.key.keysym.mod & KMOD_CTRL;
                bool32 alt_down = event.key.keysym.mod & KMOD_ALT;
                bool32 pressed = event.key.state == SDL_PRESSED ? 1 : 0;
                
                add_key(input, event.key.keysym.sym, shift_down, ctrl_down, alt_down, pressed);
            } break;
        }
        
        StartTimer("SDL_WaitEventTimeout()");
    }
    EndTimer();
    
    {
        // NOTE(Val): Get mouse state
        
        i32 x, y;
        u32 buttons = SDL_GetMouseState(&x, &y);
        
        input->mouse.old_x = input->mouse.x;
        input->mouse.old_y = input->mouse.y;
        
        input->mouse.x = x;
        input->mouse.y = y;
        
        input->mouse.left_button_was_pressed = input->mouse.left_button_is_pressed;
        input->mouse.right_button_was_pressed = input->mouse.right_button_is_pressed;
        input->mouse.middle_button_was_pressed = input->mouse.middle_button_is_pressed;
        
        input->mouse.left_button_is_pressed = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
        input->mouse.right_button_is_pressed = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
        input->mouse.middle_button_is_pressed = buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
    }
    
    EndTimer();
    EndTimer();
    RETURN(SUCCESS);
}

internal int64
PlatformGetThreadID()
{
    return SDL_ThreadID();
}

internal int32
InitializeTextures()
{
    wlog(LOG_INFO, "InitializeTextures(): Initializing SDL textures");
    
    int32 width;
    int32 height;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    SDL_GetWindowSize(window,
                      &width,
                      &height);
    
    ui_texture = SDL_CreateTexture(renderer,
                                   SDL_PIXELFORMAT_BGRA32,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width,
                                   height);
    
    SDL_SetTextureBlendMode(ui_texture,
                            SDL_BLENDMODE_BLEND);
    
    pdata->client.output_width = width;
    pdata->client.output_height = height;
    pdata->client.bytes_per_pixel = 4;
    
    RETURN(SUCCESS);
}

internal SDL_Texture *
PlatformConvertSurfaceToTexture(SDL_Surface *surface)
{
    return SDL_CreateTextureFromSurface(renderer, surface);
}

static inline void
print_usage()
{
    printf("usage: %s -i input_file [-s] [-p ip]\n "
           "\t-s:    use this machine as a streaming server\n"
           "\t-r ip: use this machine as a partner and connect to the server on the provided ip\n",
#if defined(_WIN32)
           "watchtogether.exe"
#elif defined(__linux__)
           "watchtogether"
#endif
           );
}

#ifdef _WIN32
#undef main         // workaround on windows where main is redefined by SDL
#endif

int main(int argc, char *argv[])
{
    InitializeLogger(CONFIG_LOGGING_LEVEL, "debug/log.txt");
    
    int ret = 0;
    // initialize all the necessary SDL stuff
    if(argc == 1)
    {
        print_usage();
        return 0;
    }
    
    SDL_SetMainReady();
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        wlog(LOG_FATAL, "Failed to initialize SDL.");
        
        RETURN(FAILED_TO_INIT);
    }
    
    if(SDLNet_Init() != 0)
    {
        wlog(LOG_FATAL, "Failed to initialize SDLNet");
        
        RETURN(FAILED_TO_INIT);
    }
    
    if(TTF_Init())
    {
        wlog(LOG_FATAL, "Failed to initialize TTF");
        
        RETURN(FAILED_TO_INIT);
    }
    
    //SDL_SetHint("SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP", "0");
    
    SDL_version ver, ver2;
    SDL_VERSION(&ver);
    SDL_GetVersion(&ver2);
    
    dbg_info("Version_1: %d.%d.%d\n"
             "Version_2: %d.%d.%d\n",
             ver.major,
             ver.minor,
             ver.patch,
             ver2.major,
             ver2.minor,
             ver2.patch);
    
#ifdef _WIN32
    SDL_SetEventFilter(resize_filter, NULL);
    wlog(LOG_INFO, "Setting input event filter.");
#endif
    
    ret = SDL_CreateWindowAndRenderer(1280,
                                      720,
                                      0,//SDL_WINDOW_RESIZABLE,
                                      &window,
                                      &renderer);
    SDL_Test(ret);
    
    if(window == NULL)
    {
        wlog(LOG_FATAL, "Failed to create a window.");
        
        SDL_Quit();
        return -1;
    }
    
    SDL_SetWindowTitle(window,
                       WT_WINDOW_TITLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
                "linear");
    
    
    SDL_ShowWindow(window);
    wlog(LOG_INFO, "Displaying window.");
    
    pdata = calloc(sizeof(program_data), 1);
    pdata->running = 1;
    
    if(InitializeTextures()) {
        wlog(LOG_FATAL, "Failed initializing textures.");
        RETURN(UNKNOWN_ERROR);
    }
    
    int argi = 1;
    while(argi < argc)
    {
        if(!strcmp(argv[argi], "-i"))
        {
            strcpy(pdata->file.filename, (char *)argv[++argi]);
        }
        else if(!strcmp(argv[argi], "-s") || !strcmp(argv[argi], "--server"))
        {
            if(!ReceivingFile(pdata))
            {
                SetHosting(pdata);
            }
            else
            {
                printf("You cannot both host and connect to a host.\n\n");
                print_usage();
            }
        }
        else if(!strcmp(argv[argi], "-r") || !strcmp(argv[argi], "--receiver"))
        {
            if(!HostingFile(pdata))
            {
                //pdata->file.filename = (char *)argv[++argi];
                pdata->server_address = (char *)argv[++argi];
                
                SetReceiving(pdata);
            }
            else
            {
                printf("You cannot both host and connect to a host.\n\n");
                print_usage();
            }
        }
        else if(!strcmp(argv[argi], "-h"))
        {
            print_usage();
            goto main_end;
        }
        
        argi++;
    }
    
    assert(!(HostingFile(pdata) && ReceivingFile(pdata)));
    
    SDL_DisplayMode display_info;
    // TODO(Val): For now this just picks 0th monitor. Make this better.
    SDL_GetCurrentDisplayMode(0, &display_info);
    pdata->hardware.monitor_refresh_rate = display_info.refresh_rate;
    pdata->hardware.monitor_width = display_info.w;
    pdata->hardware.monitor_height = display_info.h;
    
    wlog(LOG_INFO, "Starting main loop.");
    MainThread(pdata);
    wlog(LOG_INFO, "Exiting main loop");
    
    wlog(LOG_INFO, "Closing textures and window");
    
    main_end: 
    SDL_DestroyTexture(video_texture);
    SDL_DestroyTexture(ui_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    wlog(LOG_INFO, "Closing audio");
    SDL_CloseAudioDevice(AudioID);
    
    wlog(LOG_INFO, "Finishing logging");
    CloseLogger();
    
    TTF_Quit();
    SDL_Quit();
    return 0;
}
