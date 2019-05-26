#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "defines.h"
#include "wt_version.h"
#include "utils.h"
#include "kbkeys.h"

#include "deb-watchtogether-v2.h"
#include "watchtogether.c"

/* TODO(Val): A list of things that still must be done 
 --- Provide TCP/UDP support
 --- Touchscreen support
 --- Rewrite audio pushing so that I can push arbitrary 
 ---     lengths of audio into queue
 --- Write an allocator to handle all allocations instead of 
 ---     malloc/av_malloc
 */

global SDL_AudioDeviceID AudioID = -1;
global bool32 audio_initialized = 0;
global float ms_target = 33.33333333333333333333f;
global SDL_Window *window = NULL;
global SDL_Renderer *renderer = NULL;
global SDL_Texture *texture;
global SDL_Texture *background_texture;
global SDL_Texture *ui_texture;
global uint8 silence;
global uint32 texture_width;
global uint32 texture_height;

global program_data *pdata;

#define TESTING_FILE "data/video.mp4"

// TODO(Val): remove this, just temporary
// NOTE(Val): sets the length of a frame in ms
static void
set_FPS(float value)
{
    dbg_print("ms target set to: %f\n", value);
    ms_target = value;
}

static void
blit_frame(program_data *pdata)
{
    // TODO(Val): this is temporary, should get these values properly
    //dbg_info("blit_frame\n");
    
    
    //SDL_LockTexture(background_texture, NULL, &pixels, &pitch);
    
    if(pdata->file.video_format == VIDEO_YUV)
    {
        void* pixels;
        int32 pitch;
        
        void *Y, *U, *V;
        uint32 Yp, Up, Vp;
        int ret = get_next_frame_YUV(&pdata->vq_data,
                                     &Y, &Yp,
                                     &U, &Up,
                                     &V, &Vp);
        if(!ret)
        {
            SDL_UpdateYUVTexture(background_texture, NULL, Y, Yp, U, Up, V, Vp);
            if(!pdata->paused)
            {
                dbg_success("Discarding frame.\n");
                int ret2 = discard_next_frame_YUV(&pdata->vq_data);
                
            }
        }
        else
        {
            dbg_error("get_next_frame_YUV() failed.\n");
        }
    }
    else if(pdata->file.video_format == VIDEO_RGB)
    {
        if(!pdata->paused)
        {
            void* buffer; // = malloc(pdata->vq_data.video_queue_frame_size);
            uint32 pitch;
            int ret = get_next_frame(&pdata->vq_data, &buffer, &pitch);
            
            if(!ret)
            {
                SDL_UpdateTexture(background_texture, NULL, buffer, pitch);
                discard_next_frame(&pdata->vq_data);
            }
            else
            {
                dbg_error("dequeue_frame() failed.\n");
            }
        }
    }
    else
    {
        dbg_error("Not initialized?");
    }
    
    //SDL_UnlockTexture(background_texture);
    
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

static void
AudioCallback(void*  userdata,
              Uint8* stream,
              int    len)
{
    //program_data *pdata = userdata;
    //dbg_info("AudioCallback executed.\n");
    //void* data = malloc(len);
    int ret = dequeue_audio_bytes(&pdata->aq_data, stream, len);
    
    
    
    
    
    if(ret)
    {
        dbg_error("Something happened to audio\n");
        memset(stream, silence, len); 
    }
    else
    {
        // TODO(Val): Pull the proper format
        //SDL_MixAudioFormat(stream, data,
        //pdata->audio_format,
        //len,
        //SDL_MIX_MAXVOLUME);
        
        dbg_success("AudioCallback was successful\n");
    }
    
    //free(data);
}

/*
static void
set_audio_properties(uint32 freq,
                     uint32 channels,
                     uint32 bytes_per_sample)
{
    SDL_AudioSpec spec = {};
    spec.callback = AudioCallback;
    spec.freq = freq;
    if(channels == 1 || channels == 2 || channels == 4 || channels == 6)
        spec.channels = channels;
    else
        dbg_error("Improper number of channels when initializing audio.\n");
    spec.samples = 4096;
    
    if(bytes_per_sample == SAMPLE_U8)
        spec.format = AUDIO_U8;
    else if(bytes_per_sample == SAMPLE_S16)
        spec.format = AUDIO_S16;
    else if(bytes_per_sample == SAMPLE_S32)
        spec.format = AUDIO_S32;
    else if(bytes_per_sample == SAMPLE_FLOAT)
        spec.format = AUDIO_F32;
    else
        dbg_error("Audio format not handled.\n");
        
    // TODO(Val): Make this the determining factor for what
    // audio to push
    SDL_AudioSpec received = {};
    if(spec.format)
    {
        AudioID = SDL_OpenAudioDevice(NULL,
                                      0,
                                      &spec,
                                      &received,
                                      SDL_AUDIO_ALLOW_FORMAT_CHANGE);
                                      
        if(received.freq == spec.freq &&
           received.channels == spec.channels &&
           received.format == spec.format)
        {
            silence = received.silence;
            SDL_PauseAudioDevice(AudioID, 0);
            dbg_success("Successfully created audio device\n");
        }
        else
        {
            dbg_error("Audio device opened with differing properties.\n");
        }
    }
}
*/

/*
static void
EnqueueAudio(sound_sample *SoundSample)
{
    // NOTE(Val): This is split into two parts because 
    // it currently loops the audio non-stop
    if(SoundSample->Size)
    {
        uint32 ret = SDL_QueueAudio(AudioID,
                                    SoundSample->Data,
                                    SoundSample->Size);
        if(ret < 0)
            dbg_error("Audio queueing failed!\n");
    }
    //dbg_print("EnqueueAudio: %s\n", SDL_GetError());
    //dbg_print("Current queue size: %d\n", SDL_GetQueuedAudioSize(AudioID));
}
*/

static void 
PlatformInitAudio(program_data *pdata)
{
    dbg_info("PlatformInitAudio called.\n");
    
    open_file_info *file = &pdata->file;
    output_audio *output = &pdata->audio;
    
    SDL_AudioSpec DesiredAudioSpec = {};
    SDL_AudioSpec ReceivedAudioSpec = {};
    uint32 bytes_per_sample = 0;
    
    DesiredAudioSpec.freq = file->sample_rate;
    
    // NOTE(Val): Should depend on source audio
    if(file->sample_format == SAMPLE_U8)
    {
        DesiredAudioSpec.format = AUDIO_U8;
        bytes_per_sample = 1;
    }
    else if(file->sample_format == SAMPLE_S16)
    {
        DesiredAudioSpec.format = AUDIO_S16;
        bytes_per_sample = 2;
    }
    else if(file->sample_format == SAMPLE_S32)
    {
        DesiredAudioSpec.format = AUDIO_S32;
        bytes_per_sample = 4;
    }
    else if(file->sample_format == SAMPLE_FLOAT)
    {
        DesiredAudioSpec.format = AUDIO_F32;
        bytes_per_sample = 4;
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
    DesiredAudioSpec.callback = AudioCallback;
    DesiredAudioSpec.userdata = pdata;
    
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
PlatformPauseAudio(bool32 b)
{
    SDL_PauseAudioDevice(AudioID, b);
}

static void
PlatformSleep(int32 ms)
{
    SDL_Delay(ms);
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

static int32
ProcessInput(void *arg)
{
    program_data *pdata = arg;
    input_struct *input = &pdata->input;
    
    // TODO(Val): aggregate input, as we don't know how often the application
    // will check it
    SDL_Event event = {};
    while(pdata->running && SDL_WaitEvent(&event))
    {
        //dbg_info("Event received.\n");
        switch(event.type)
        {
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        /*
int width = pdata->vq_data.video_queue_width;
                        int height = pdata->vq_data.video_queue_height;
                        
                        SDL_Rect rect;
                        SDL_RenderGetViewport(renderer, &rect);
                        
                        real32 width_ratio = 1.0f*rect.w/width;
                        real32 height_ratio = 1.0f*rect.h/height;
                        
                        //real32 new_aspect = (1.0f * rect.w) / rect.h;
                        real32 aspect = (1.0f * pdata->vq_data.video_queue_width) / pdata->vq_data.video_queue_height;
                        
                        if(width_ratio > height_ratio)
                        {
                            SDL_RenderSetLogicalSize(renderer, height_ratio*width, rect.h);
                        }
                        else if(height_ratio < width_ratio)
                        {
                            SDL_RenderSetLogicalSize(renderer, rect.w, width_ratio*height);
                        }
                        else
                        {
                            SDL_RenderSetLogicalSize(renderer, rect.w, rect.h);
                        }
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
            {
                // NOTE(Val): Keypress events here
                switch(event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                    {
                        TogglePlayback(pdata);
                        
                    } break;
                    case SDLK_ESCAPE:
                    {
                        pdata->running = 0;
                    } break;
                    case SDLK_RETURN:
                    {
                        SDL_Keymod mod = SDL_GetModState();
                        
                        if(mod & KMOD_ALT)
                        {
                            dbg_error("ALT + Enter pressed.\n");
                            if(pdata->client.fullscreen)
                            {
                                SDL_SetWindowFullscreen(window, 0);
                                pdata->client.fullscreen = 0;
                            }
                            else
                            {
                                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                                pdata->client.fullscreen = 1;
                            }
                        }
                        else if(mod & KMOD_CTRL)
                        {
                            
                        }
                        else if(mod & KMOD_SHIFT)
                        {
                            
                        }
                    } break;
                    default:
                    {
                        
                    } break;
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
            default: 
            {
                
            } break;
        }
    }
    
    return 0;
}

static uint32
PlatformGetTime()
{
    return SDL_GetTicks();
}

// TODO(Val): This still needs to be redone, as the UI should probably
// update outside of the video fps target
static int32
PlatformFrameUpdater(void *data)
{
    program_data *pdata = data;
    
    //dbg_info("PlatformFrameUpdater loop start.\n");
    dbg_print("%s\n", SDL_GetError());
    
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
    
    int ret;
    
    if(pdata->playing)
        blit_frame(pdata);
    
    //SDL_RenderSetScale(renderer, texture_width, texture_height);
    ret = SDL_RenderCopy(renderer, background_texture, NULL, NULL);
    if(ret)
    {
        dbg_error(SDL_GetError());
        dbg_print("\n");
    }
    //SDL_RenderCopy(renderer, ui_texture, NULL, NULL);
    
    SDL_RenderPresent(renderer);
    
    return 0;
}

static void
PlatformInitVideo(program_data *pdata)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    
    if(pdata->file.video_format == VIDEO_RGB)
    {
        background_texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_BGRA32,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               pdata->file.width,
                                               pdata->file.height);
    }
    else if(pdata->file.video_format == VIDEO_YUV)
    {
        background_texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_IYUV,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               pdata->file.width,
                                               pdata->file.height);
    }
    
    SDL_SetTextureBlendMode(background_texture, SDL_BLENDMODE_NONE);
    
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    
    SDL_RenderSetViewport(renderer, NULL);
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

static int PlatformGetInput(program_data *pdata)
{
    input_struct *input = &pdata->input;
    
    SDL_Event event = {};
    
    //while(pdata->running && SDL_PollEvent(&event))
    SDL_PumpEvents();
    while(pdata->running &&
          input->keyboard.n < MAX_KEYS-1 &&
          SDL_PollEvent(&event))
    {
        //dbg_info("Event received.\n");
        switch(event.type)
        {
            case SDL_QUIT:
            {
                pdata->running = 0;
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
                
#define add_key(a, b) add_key(input, a, shift_down, ctrl_down, alt_down, b)
                // NOTE(Val): Keypress events here
                switch(event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                    {
                        add_key(KB_SPACE, pressed);
                    } break;
                    case SDLK_ESCAPE:
                    {
                        add_key(KB_ESCAPE, pressed);
                    } break;
                    case SDLK_RETURN:
                    {
                        add_key(KB_ENTER, pressed);
                    } break;
                    case SDLK_F4:
                    {
                        add_key(KB_F4, pressed);
                    } break;
                    case SDLK_UP:
                    {
                        add_key(KB_UP, pressed);
                    } break;
                    case SDLK_DOWN:
                    {
                        add_key(KB_DOWN, pressed);
                    } break;
                    /*
                    case SDLK_SPACE:
                    {
                    TogglePlayback(pdata);
                    
                    } break;
                    case SDLK_ESCAPE:
                    {
                    pdata->running = 0;
                    } break;
                    case SDLK_RETURN:
                    {
                    SDL_Keymod mod = SDL_GetModState();
                    
                    if(mod & KMOD_ALT)
                    {
                    dbg_error("ALT + Enter pressed.\n");
                    if(pdata->client.fullscreen)
                    {
                    SDL_SetWindowFullscreen(window, 0);
                    pdata->client.fullscreen = 0;
                    }
                    else
                    {
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    pdata->client.fullscreen = 1;
                    }
                    }
                    else if(mod & KMOD_CTRL)
                    {
                    
                    }
                    else if(mod & KMOD_SHIFT)
                    {
                    
                    }
                    } break;
                    default:
                    {
                    
                    } break;
                    */
                }
#undef add_key
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
            default: 
            {
                
            } break;
        }
        
        // TODO(Val): Time here
    }
    
    return 0;
}

#ifdef _WIN32
#undef main         // workaround on windows where main is redefined by SDL
#endif

int main(int argc, const char* argv[])
{
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
        return 1;
    }
    
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
    
    //Deb_ResizePixelBuffer(renderer);
    
    //pdata->threads.main_thread = PlatformCreateThread(MainLoop, pdata, "main");
    MainThread(pdata);
    
    //ProcessInput(pdata);
    
    // TODO(Val): Close everything properly here
    PlatformWaitThread(pdata->threads.main_thread, NULL);
    
    dbg_info("Cleaning up.\n");
    
    free(pdata);
    
    SDL_DestroyTexture(texture);
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(ui_texture);
    
    SDL_CloseAudioDevice(AudioID);
    
    SDL_Quit();
    return 0;
}
