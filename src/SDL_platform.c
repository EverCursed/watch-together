#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <assert.h>

#include "watchtogether.h"
#include "utils/timing.h"
#include "utils.h"

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
 --- Provide TCP/UDP support
 --- Touchscreen support
 --- Write an allocator to handle all allocations instead of 
 ---     malloc/av_malloc
 */

global SDL_AudioDeviceID AudioID = -1;
global bool32 audio_initialized = 0;
global SDL_Window *window = NULL;
//global SDL_Renderer *renderer = NULL;
//global SDL_Texture *background_texture;
//global SDL_Texture *ui_texture;
global SDL_Surface *final_surface = NULL;
global SDL_Surface *video_surface = NULL;
global SDL_Surface *ui_surface = NULL;

global program_data *pdata;

#define TESTING_FILE "data/video_test/video.mp4"

/*
static void 
Deb_ResizePixelBuffer(SDL_Renderer *renderer)
{
    int width, height;
    
    SDL_GetRendererOutputSize(renderer,
                              &width,
                              &height);
                              
    texture_height = height;
    texture_width = width;
    
    dbg_print("Renderer width: %d,\theight: %d\n", width, height);
    
    if(background_texture)
        SDL_DestroyTexture(background_texture);
        
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
}
*/ 

static inline int32
LockSurface(SDL_Surface *surface)
{
    int32 ret = 0;
    StartTimer("LockSurface");
    
    if(SDL_MUSTLOCK(surface))
        ret = SDL_LockSurface(surface);
    
    EndTimer();
    return ret;
}

static inline void
UnlockSurface(SDL_Surface *surface)
{
    StartTimer("UnlockSurface");
    
    if(SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
    
    EndTimer();
}

int32
PlatformQueueAudio(output_audio *audio)
{
    int ret = SDL_QueueAudio(AudioID,
                             audio->buffer,
                             audio->size);
    
    if(ret < 0)
    {
        dbg_error("%s\n", SDL_GetError());
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

/*
static void
AudioCallback(void*  userdata,
              Uint8* stream,
              int    len)
{
    output_audio *audio = &pdata->audio;
    if(audio->is_ready)
    {
        dbg_success("AudioCallback was successful\n");
        
    }
    else
    {
        // TODO(Val): Pull the proper format
        //SDL_MixAudioFormat(stream, data,
        //pdata->audio_format,
        //len,
        //SDL_MIX_MAXVOLUME);
        
        dbg_error("Something happened to audio\n");
        memset(stream, silence, len); 
    }
}
*/
int32
PlatformInitAudio(program_data *pdata)
{
    //dbg_info("PlatformInitAudio called.\n");
    
    open_file_info *file = &pdata->file;
    output_audio *output = &pdata->audio;
    
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
        dbg_error("An unhandled audio format was passed.\n");
        RETURN(UNKNOWN_FORMAT);
    }
    
    DesiredAudioSpec.channels = file->channels;
    
    // NOTE(Val): Sending a number of samples to the audio device
    // prevents you from immediately closing the app until the
    // entire buffer is played. Therefore this is small.
    // TODO(Val): Test to see what the smallest value we can set 
    // to and how that would affect performance.
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
        
        output->sample_rate = ReceivedAudioSpec.freq;
        output->channels = ReceivedAudioSpec.channels;
        
        // TODO(Val): This doesnt check what audio device params we received.
        output->bytes_per_sample = bytes_per_sample;
        output->sample_format = file->sample_format;
        
        pdata->audio_format = ReceivedAudioSpec.format;
        
        //SDL_PauseAudioDevice(AudioID, 0);
    }
    else
    {
        dbg_error("SDL_OpenAudioDevice() failed.\n");
        
        RETURN(UNKNOWN_ERROR);
    }
    //printf("%s\n", SDL_GetError());
    
    RETURN(SUCCESS);
}

void
PlatformCloseAudio(program_data *pdata)
{
    SDL_CloseAudioDevice(AudioID);
}

void
PlatformPauseAudio(bool32 b)
{
    SDL_PauseAudioDevice(AudioID, b);
}
/*
void
PlatformSleep(real64 s)
{
    int st = s < 0 ? 0 : s * 1000.0;
    SDL_Delay(st);
}
*/
/// Platform create thread
thread_info
PlatformCreateThread(int32 (*f)(void *), void *data, char* name)
{
    thread_info info = {};
    char* title = name == NULL ? "" : name;
    
    info.thread = SDL_CreateThread(f,
                                   title,
                                   data);
    
    return info;
}

void
PlatformWaitThread(thread_info thread, int32 *ret)
{
    SDL_WaitThread(thread.thread, ret);
}

cond_info
PlatformCreateConditionVar()
{
    cond_info c = {};
    c.mutex = SDL_CreateMutex();
    c.cond = SDL_CreateCond();
    c.test = 0;
    return c;
}

int32
PlatformConditionWait(cond_info *c)
{
    
    if(SDL_LockMutex(c->mutex))
        RETURN(UNKNOWN_ERROR);
    
    while(!c->test && pdata->running)
    {
        SDL_CondWait(c->cond, c->mutex);
        //PlatformSleep(0.001);
    }
    c->test = 0;
    SDL_UnlockMutex(c->mutex);
    
    
    //while(!c->test && pdata->running)
    //{
    //PlatformSleep(0.001);
    //}
    //c->test = 0;
    RETURN(SUCCESS);
}

int32
PlatformConditionSignal(cond_info *c)
{
    if(SDL_LockMutex(c->mutex))
        RETURN(UNKNOWN_ERROR);
    
    c->test = 1;
    SDL_CondSignal(c->cond);
    SDL_UnlockMutex(c->mutex);
    
    //c->test = 1;
    RETURN(SUCCESS);
}

int32
PlatformConditionDestroy(cond_info *c)
{
    SDL_DestroyMutex(c->mutex);
    SDL_DestroyCond(c->cond);
    
    RETURN(SUCCESS);
}

platform_mutex
PlatformCreateMutex()
{
    platform_mutex m = {};
    m.mutex = SDL_CreateMutex();
    if(m.mutex == NULL)
        dbg_error("%s\n", SDL_GetError());
    return m;
}

int32
PlatformLockMutex(platform_mutex *m)
{
    if(!SDL_LockMutex(m->mutex))
        RETURN(SUCCESS);
    else
    {
        dbg_error("%s\n", SDL_GetError());
        RETURN(UNKNOWN_ERROR);
    }
}

int32
PlatformUnlockMutex(platform_mutex *m)
{
    if(!SDL_UnlockMutex(m->mutex))
        RETURN(SUCCESS);
    else
        RETURN(UNKNOWN_ERROR);
}

/*
real64
PlatformGetTime()
{
    real64 ticks = (real64)SDL_GetTicks();
    dbg_info("PlatformGetTime(): %lf\n", ticks/1000.0);
    return ticks/1000.0;
}
*/
int32
PlatformUpdateVideoFrame(program_data *pdata)
{
    output_video *video = &pdata->video;
    
    int ret = 0;
    
    StartTimer("PlatformUpdateVideoFrame()");
    
    ret = LockSurface(video_surface);
    if(ret)
        dbg_error("%s\n", SDL_GetError());
    
    int32 pitch = video_surface->pitch;
    void *data = video_surface->pixels;
    
    StartTimer("memcpy");
    for(int i = 0; i < video->height; i++)
    {
        memcpy(data + i*pitch, video->video_frame + i*video->pitch, video->pitch);
    }
    EndTimer();
    
    UnlockSurface(video_surface);
    
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

int32
PlatformUpdateFrame(program_data *pdata)
{
    StartTimer("PlatformUpdateFrame");
    
    int ret = 0;
    
    StartTimer("SDL_FillRect");
    ret = SDL_FillRect(SDL_GetWindowSurface(window),
                       NULL,
                       0);
    EndTimer();
    
    if(video_surface)
    {
        if(ret)
        {
            dbg_error("%s\n", SDL_GetError());
            EndTimer();
            RETURN(UNKNOWN_ERROR);
        }
        
        StartTimer("SDL_BlitScaled(video_surface)");
        ret = SDL_BlitScaled(video_surface,
                             NULL,
                             SDL_GetWindowSurface(window),
                             NULL);
        EndTimer();
    }
    
    
    StartTimer("SDL_BlitSurface(ui_surface)");
    ret = SDL_BlitSurface(ui_surface,
                          NULL,
                          SDL_GetWindowSurface(window),
                          NULL);
    EndTimer();
    if(ret)
    {
        dbg_error("%s\n", SDL_GetError());
        EndTimer();
        RETURN(UNKNOWN_ERROR);
    }
    
    // Render the UI on top of video frame
    
    EndTimer();
    RETURN(SUCCESS);
}

int32
PlatformFlipBuffers(program_data *pdata)
{
    SDL_UpdateWindowSurface(window);
    
    RETURN(SUCCESS);
}

void
PlatformInitVideo(program_data *pdata)
{
    if(video_surface)
        SDL_FreeSurface(video_surface);
    
    video_surface = SDL_CreateRGBSurfaceWithFormat(0, pdata->file.width, pdata->file.height, 32, SDL_PIXELFORMAT_BGRA32);
    //SDL_SetSurfaceRLE(video_surface, 1);
}

void
PlatformToggleFullscreen(program_data *pdata)
{
    dbg_success("TOGGLING FULLSCREEN\n");
    SDL_SetWindowFullscreen(window, pdata->is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
    pdata->is_fullscreen = !pdata->is_fullscreen;
}

static inline void add_key(input_struct *input,
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
int resize_filter(void *userdata,
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

static int32
ResizeScreen(program_data *pdata, int x, int y)
{
    StartTimer("ResizeScreen()");
    
    uint32 rmask, gmask, bmask, amask;
    
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    
    if(ui_surface)
        SDL_FreeSurface(ui_surface);
    
    ui_surface = SDL_CreateRGBSurface(0, x, y, 32,
                                      rmask, gmask, bmask, amask);
    //SDL_SetSurfaceRLE(ui_surface, 1);
    SDL_SetSurfaceBlendMode(ui_surface, SDL_BLENDMODE_BLEND);
    
    EndTimer();
    
    RETURN(SUCCESS);
}

int32
PlatformGetInput(program_data *pdata)
{
    StartTimer("PlatformGetInput()");
    
    input_struct *input = &pdata->input;
    
    SDL_Event event = {0};
    
    StartTimer("Event loop");
    StartTimer("SDL_WaitEventTimeout()");
    while(pdata->running &&
          input->keyboard.n < MAX_KEYS-1 &&
          SDL_WaitEventTimeout(&event, 15))
    {
        EndTimer();
        //dbg_info("Event received.\n");
        switch(event.type)
        {
            case SDL_QUIT:
            {
                dbg_error("SDL_QUIT\n");
                pdata->running = 0;
                RETURN(SUCCESS);
            } break;
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        dbg_error("SDL_WINDOWEVENT_RESIZED fired.\n");
                        
                        ResizeScreen(pdata, event.window.data1, event.window.data2);
                        /*
                        int width = pdata->vq_data.video_queue_width;
                        int height = pdata->vq_data.video_queue_height;
                        
                        SDL_RenderGetViewport(renderer, &rect);
                        
                        */
                        
                        //Deb_ResizePixelBuffer(renderer);
                        
                        
                        //dbg_print("Texture width: %d\theight: %d\n", texture_width, texture_height);
                        //dbg_print("Scale x: %f\t y: %f\n", scaleX, scaleY);
                        
                        
                        //SDL_FreeSurface(surface);
                        //surface = Deb_ResizePixelBuffer(window);
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
                
                if(event.key.keysym.sym == KB_F4 && alt_down)
                {
                    PlatformToggleFullscreen(pdata);
                    PlatformFlipBuffers(pdata);
                }
                else
                {
                    add_key(input, event.key.keysym.sym, shift_down, ctrl_down, alt_down, pressed);
                }
            } break;
            case SDL_MOUSEMOTION:
            {
                input->mouse.old_x = input->mouse.x;
                input->mouse.old_y = input->mouse.y;
                
                input->mouse.x = event.motion.x;
                input->mouse.y = event.motion.y;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    input->mouse.left_button_was_pressed = input->mouse.left_button_is_pressed;
                    input->mouse.left_button_is_pressed = event.button.state == SDL_PRESSED ? 1 : 0;
                }
                else if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    input->mouse.right_button_was_pressed = input->mouse.right_button_is_pressed;
                    input->mouse.right_button_is_pressed = event.button.state == SDL_PRESSED ? 1 : 0;
                }
                else if(event.button.button == SDL_BUTTON_MIDDLE)
                {
                    input->mouse.middle_button_was_pressed = input->mouse.middle_button_is_pressed;
                    input->mouse.middle_button_is_pressed = event.button.state == SDL_PRESSED ? 1 : 0;
                }
                else
                {
                    dbg_info("Unhandled mouse button input.\n");
                }
            } break;
        }
        
        StartTimer("SDL_WaitEventTimeout()");
    }
    EndTimer();
    
    EndTimer();
    EndTimer();
    RETURN(SUCCESS);
}

int64
PlatformGetThreadID()
{
    return SDL_ThreadID();
}

static int32 InitializeSurfaces(int width, int height)
{
    if(ui_surface)
        SDL_FreeSurface(ui_surface);
    
    ui_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_BGRA32);
    if (ui_surface == NULL) {
        dbg_error("SDL_CreateRGBSurface: %s\n", SDL_GetError());
        
        RETURN(UNKNOWN_ERROR);
    }
    SDL_SetSurfaceBlendMode(ui_surface, SDL_BLENDMODE_BLEND);
    //SDL_SetSurfaceRLE(ui_surface, 1);
    
    pdata->client.output_width = width;
    pdata->client.output_height = height;
    pdata->client.bytes_per_pixel = 4;
    
    RETURN(SUCCESS);
}

#ifdef _WIN32
#undef main         // workaround on windows where main is redefined by SDL
#endif

int main(int argc, char *argv[])
{
    // initialize all the necessary SDL stuff
    if(argc == 1)
    {
        printf("usage: %s -i input_file [-s] [-p ip]\n "
               "\t-s:    use this machine as a streaming server\n"
               "\t-p ip: use this machine as a partner and connect to the server on the provided ip\n",
               argv[0]);
        return 0;
    }
    
    SDL_SetMainReady();
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        RETURN(FAILED_TO_INIT);
    }
    
    if(SDLNet_Init() != 0)
    {
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
    dbg_info("SDL_FilterEvents\n");
    SDL_SetEventFilter(resize_filter, NULL);
#endif
    
    // TODO(Val): Check if this actually gives us the proper client area
    window = SDL_CreateWindow(
        WT_WINDOW_TITLE,                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        1280,                               // width, in pixels
        720,                               // height, in pixels
        SDL_WINDOW_RESIZABLE
        );
    
    if(window == NULL)
    {
        dbg_error("Creating window failed!\n");
        SDL_Quit();
        return -1;
    }
    
    SDL_ShowWindow(window);
    
    pdata = calloc(sizeof(program_data), 1);
    pdata->running = 1;
    
    if(InitializeSurfaces(1280, 720))
        RETURN(UNKNOWN_ERROR);
    
    int argi = 1;
    while(argi < argc)
    {
        if(!strcmp(argv[argi], "-i"))
        {
            pdata->file.filename = (char *)argv[++argi];
        }
        else if(!strcmp(argv[argi], "-s") || !strcmp(argv[argi], "--server"))
        {
            if(!pdata->is_partner)
            {
                pdata->is_host = 1;
            }
            else
            {
                // TODO(Val): Warning 
            }
        }
        else if(!strcmp(argv[argi], "-p") || !strcmp(argv[argi], "--partner"))
        {
            if(!pdata->is_host)
            {
                pdata->is_partner = 1;
            }
            else
            {
                // TODO(Val): Warning
            }
        }
        
        argi++;
    }
    
    assert(!(pdata->is_host && pdata->is_partner));
    
    /*
    if(argc > 1)
        pdata->file.filename = (char *)*(argv+1);
    else
        pdata->file.filename = TESTING_FILE;
    */
    SDL_DisplayMode display_info;
    // TODO(Val): For now this just picks 0th monitor. Make this better.
    SDL_GetCurrentDisplayMode(0, &display_info);
    pdata->hardware.monitor_refresh_rate = display_info.refresh_rate;
    pdata->hardware.monitor_width = display_info.w;
    pdata->hardware.monitor_height = display_info.h;
    
    MainThread(pdata);
    
    //FinishTiming;
    //ProcessInput(pdata);
    
    // TODO(Val): Close everything properly here
    //PlatformWaitThread(pdata->threads.main_thread, NULL);
    
    dbg_info("Cleaning up.\n");
    
    SDL_DestroyWindow(window);
    
    //free(pdata);
    // TODO(Val): Are these necessary? 
    
    SDL_CloseAudioDevice(AudioID);
    
    SDL_Quit();
    return 0;
}
