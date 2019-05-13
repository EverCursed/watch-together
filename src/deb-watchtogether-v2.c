#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "defines.h"
#include "wt_version.h"
#include "utils.h"

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

#define TESTING_FILE "data/5cm.mkv"

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
    //dbg_info("AudioCallback executed.\n");
    int ret = dequeue_audio_bytes(&pdata->aq_data, stream, len);
    
    if(ret)
    {
        dbg_error("Something happened to audio\n");
        dbg_print("ret: %d\n", ret);
        memset(stream, silence, len); 
    }
    else
    {
        dbg_success("AudioCallback was successful\n");
    }
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
    DesiredAudioSpec.userdata = NULL;
    
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
PlatformSleep(uint32 ms)
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

static struct timespec
time_diff(struct timespec t2, struct timespec t1)
{
    struct timespec ret = {};
    if ((t2.tv_nsec - t1.tv_nsec) < 0) {
        ret.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        ret.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        ret.tv_sec = t2.tv_sec - t1.tv_sec;
        ret.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return ret;
}

// TODO(Val): This still needs to be redone, as the UI should probably
// update outside of the video fps target
static int32
PlatformFrameUpdater(void *data)
{
    program_data *pdata = data;
    
    while(pdata->running)
    {
        //dbg_info("PlatformFrameUpdater loop start.\n");
        
        struct timespec TimeStart, TimeEnd;
        
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
        
        
        clock_gettime(CLOCK_REALTIME, &TimeStart);
        
        if(pdata->playing)
            blit_frame(pdata);
        
        //SDL_RenderSetScale(renderer, texture_width, texture_height);
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);
        //SDL_RenderCopy(renderer, ui_texture, NULL, NULL);
        
        SDL_RenderPresent(renderer);
        
        
        
        
        clock_gettime(CLOCK_REALTIME, &TimeEnd);
        
        // TODO(Val): Rewrite the sleep for frame updater, maybe use SDL stuff somehow?
        struct timespec TimeDifference = time_diff(TimeEnd, TimeStart);
        //dbg_print("TimeDiff: tv_sec = %ld\ttv_nsec = %ld\n", TimeDifference.tv_sec, TimeDifference.tv_nsec);
        
        struct timespec target = {};
        target.tv_sec = (uint64)(ms_target / 1000.0f);
        target.tv_nsec = (uint64)(ms_target * 1000000.0f);
        
        //dbg_print("Target: tv_sec = %ld\ttv_nsec = %ld\n", target.tv_sec, target.tv_nsec);
        
        struct timespec SleepDuration = time_diff(target, TimeDifference);
        
        //dbg_print("Nanosleep: tv_sec = %ld\ttv_nsec = %ld\n", SleepDuration.tv_sec, SleepDuration.tv_nsec);
        nanosleep(&SleepDuration, NULL);
    }
    
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

int main(int argc, const char** argv)
{
    // initialize all the necessary SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
        return 1;
    }
    
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
    
    //sound_sample SoundSample = load_WAV("data/audio_test/audio.wav");
    
    // NOTE(Val): av_register_all();
    
    //SDL_PauseAudioDevice(AudioID, 0); /* start audio playing. */
    
    //texture = Deb_ResizePixelBuffer(window, renderer);
    
    pdata = calloc(sizeof(program_data), 1);
    pdata->running = 1;
    
    if(argc > 1)
        pdata->file.filename = (char *)*(argv+1);
    else
        pdata->file.filename = TESTING_FILE;
    
    //Deb_ResizePixelBuffer(renderer);
    
    pdata->threads.main_thread = PlatformCreateThread(MainLoop, pdata, "main");
    
    ProcessInput(pdata);
    
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
