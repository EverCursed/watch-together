#include <stdio.h>
#include <string.h>
#include <time.h>

#include "defines.h"
#include "wt_version.h"
#include "utils.h"
#include "kbkeys.h"

#include "SDL_platform.h"
#include "watchtogether.c"

/* TODO(Val): A list of things that still must be done 
 --- Provide TCP/UDP support
 --- Touchscreen support
 --- Write an allocator to handle all allocations instead of 
 ---     malloc/av_malloc
 */

global SDL_AudioDeviceID AudioID = -1;
global bool32 audio_initialized = 0;
global float ms_target = 33.33333333333333333333f;
global SDL_Window *window = NULL;
global SDL_Renderer *renderer = NULL;
global SDL_Texture *background_texture;
global SDL_Texture *ui_texture;
global uint8 silence;
global uint32 texture_width;
global uint32 texture_height;

global program_data *pdata;

#define TESTING_FILE "data/video_test/video.mp4"

static void
blit_frame(program_data *pdata)
{
    output_video *video = &pdata->video;
    
    if(pdata->video.type == VIDEO_YUV)
    {
        int ret = SDL_UpdateYUVTexture(background_texture, NULL,
                                       video->video_frame, video->pitch,
                                       video->video_frame_sup1, video->pitch_sup1,
                                       video->video_frame_sup2, video->pitch_sup2);
        
        free(video->video_frame);
        free(video->video_frame_sup1);
        free(video->video_frame_sup2);
        
        if(ret < 0)
            goto error;
    }
    else if(pdata->video.type == VIDEO_RGB)
    {
        int ret = SDL_UpdateTexture(background_texture, NULL, video->video_frame, video->pitch);
        free(video->video_frame);
        
        if(ret < 0)
            goto error;
    }
    else
    {
        dbg_error("Video output frame not initialized?");
    }
    
    return;
    
    error:
    dbg_error("%s\n", SDL_GetError());
    return;
}

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

static int32
PlatformQueueAudio(output_audio *audio)
{
    int ret = SDL_QueueAudio(AudioID,
                             audio->buffer,
                             audio->size);
    
    if(ret < 0)
    {
        dbg_error("%s\n", SDL_GetError());
        
        return -1;
    }
    
    return 0;
}

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

static void 
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
        return;
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
        
        SDL_PauseAudioDevice(AudioID, 0);
    }
    //printf("%s\n", SDL_GetError());
}

static void
PlatformCloseAudio(program_data *pdata)
{
    SDL_CloseAudioDevice(AudioID);
}

static void
PlatformPauseAudio(bool32 b)
{
    SDL_PauseAudioDevice(AudioID, b);
}

static void
PlatformSleep(real64 s)
{
    int st = s < 0 ? 0 : s * 1000.0;
    SDL_Delay(st);
}

/// Platform create thread
static thread_info
PlatformCreateThread(int32 (*f)(void *), void *data, char* name)
{
    thread_info info = {};
    char* title = name == NULL ? "" : name;
    
    info.thread = SDL_CreateThread(f,
                                   title,
                                   data);
    
    return info;
}

static int32
PlatformWaitThread(thread_info thread, int32 *ret)
{
    SDL_WaitThread(thread.thread, ret);
}

static cond_info
PlatformCreateConditionVar()
{
    cond_info c = {};
    c.mutex = SDL_CreateMutex();
    c.cond = SDL_CreateCond();
    c.test = 0;
    return c;
}

static int32
PlatformConditionWait(cond_info *c)
{
    SDL_LockMutex(c->mutex);
    while(!c->test)
    {
        SDL_CondWait(c->cond, c->mutex);
    }
    c->test = 0;
    SDL_UnlockMutex(c->mutex);
    
    return 0;
}

static int32
PlatformConditionSignal(cond_info *c)
{
    SDL_LockMutex(c->mutex);
    c->test = 1;
    SDL_CondSignal(c->cond);
    SDL_UnlockMutex(c->mutex);
    
    return 0;
}

static bool32
PlatformConditionDestroy(cond_info *c)
{
    SDL_DestroyMutex(c->mutex);
    SDL_DestroyCond(c->cond);
    
    return 0;
}

static real64
PlatformGetTime()
{
    real64 ticks = (real64)SDL_GetTicks();
    dbg_info("PlatformGetTime(): %lf\n", ticks);
    return ticks/1000.0f;
}

static int32
PlatformUpdateFrame(program_data *pdata)
{
    int ret;
    
    blit_frame(pdata);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    void* pixels;
    int pitch; 
    
    SDL_LockTexture(background_texture, NULL, &pixels, &pitch);
    ret = SDL_RenderCopy(renderer, background_texture, NULL, NULL);
    if(ret)
    {
        dbg_error("%s\n", SDL_GetError());
    }
    SDL_UnlockTexture(background_texture);
    
    // Render the UI on top of video frame
    //SDL_RenderCopy(renderer, ui_texture, NULL, NULL);
    
    return 0;
}

static int32
PlatformFlipBuffers(program_data *pdata)
{
    SDL_RenderPresent(renderer);
    
    return 0;
}

static void
PlatformInitVideo(program_data *pdata)
{
    if(pdata->video.type == VIDEO_RGB)
    {
        background_texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_BGRA32,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               pdata->file.width,
                                               pdata->file.height);
    }
    else if(pdata->file.video_format = VIDEO_YUV)
    {
        background_texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_IYUV,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               pdata->file.width,
                                               pdata->file.height);
    }
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_SetTextureBlendMode(background_texture, SDL_BLENDMODE_NONE);
}

static void
PlatformToggleFullscreen(program_data *pdata)
{
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

static int
PlatformGetInput(program_data *pdata)
{
    input_struct *input = &pdata->input;
    
    SDL_Event event = {0};
    
    //while(pdata->running && SDL_PollEvent(&event))
    //SDL_PumpEvents();
    while(pdata->running &&
          input->keyboard.n < MAX_KEYS-1 &&
          SDL_PollEvent(&event))
    {
        //dbg_info("Event received.\n");
        switch(event.type)
        {
            case SDL_QUIT:
            {
                dbg_error("SDL_QUIT\n");
                pdata->running = 0;
                return 0;
            } break;
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        dbg_error("SDL_WINDOWEVENT_RESIZED fired.\n");
                        
                        int new_width = event.window.data1;
                        int new_height = event.window.data2;
                        
                        SDL_Rect rect = {};
                        
                        real32 width_ratio = (real32)new_width/pdata->file.width;
                        real32 height_ratio = (real32)new_height/pdata->file.height;
                        
                        real32 ratio = (real32)pdata->file.width / (real32)pdata->file.height;
                        real32 new_ratio = (real32)new_width / (real32)new_height;
                        
                        if(new_ratio >= ratio)
                        {
                            rect.h = new_height;
                            rect.w = (real32)pdata->file.width * height_ratio;
                        }
                        else
                        {
                            rect.w = new_width;
                            rect.h = (real32)pdata->file.height * width_ratio;
                        }
                        
                        rect.x = (new_width - rect.w)/2;
                        rect.y = (new_height - rect.h)/2;
                        
                        SDL_RenderSetViewport(renderer,
                                              &rect);
                        
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
                
                add_key(input, event.key.keysym.sym, shift_down, ctrl_down, alt_down, pressed);
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
    }
    
    return 0;
}

#ifdef _WIN32
#undef main         // workaround on windows where main is redefined by SDL
#endif

int main(int argc, const char** argv)
{
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
        return 1;
    }
    
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
    
    SDL_CreateWindowAndRenderer(1024,
                                576,
                                SDL_WINDOW_RESIZABLE,
                                &window,
                                &renderer);
    
    
    if(window == NULL || renderer == NULL)
    {
        dbg_error("Creating window failed!\n");
        SDL_Quit();
        return -1;
    }
    SDL_SetWindowTitle(window, WT_WINDOW_TITLE);
    SDL_ShowWindow(window);
    
    //SDL_PauseAudioDevice(AudioID, 0); /* start audio playing. */
    
    //texture = Deb_ResizePixelBuffer(window, renderer);
    
    pdata = calloc(sizeof(program_data), 1);
    pdata->running = 1;
    
    if(argc > 1)
        pdata->file.filename = (char *)*(argv+1);
    else
        pdata->file.filename = TESTING_FILE;
    
    SDL_DisplayMode display_info;
    // TODO(Val): For now this just picks 0th monitor. Make this better.
    SDL_GetCurrentDisplayMode(0, &display_info);
    pdata->hardware.monitor_refresh_rate = display_info.refresh_rate;
    pdata->hardware.monitor_width = display_info.w;
    pdata->hardware.monitor_height = display_info.h;
    
    
    MainThread(pdata);
    
    //ProcessInput(pdata);
    
    // TODO(Val): Close everything properly here
    //PlatformWaitThread(pdata->threads.main_thread, NULL);
    
    dbg_info("Cleaning up.\n");
    
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    
    //free(pdata);
    // TODO(Val): Are these necessary? 
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(ui_texture);
    
    SDL_CloseAudioDevice(AudioID);
    
    SDL_Quit();
    return 0;
}
