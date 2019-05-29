#include "watchtogether.h"
#include "platform.h"
#include "kbkeys.h"

#include "audio_queue.c"
#include "video_queue.c"
#include "decoding.c"

#include <time.h>

// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

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

// TODO(Val): Find a way to get the refresh rate of the screen, for now this is a define
#define REFRESH_RATE     16.666666666666f
// TODO(Val): Test a variety of these, and see how long it's possible to go
// NOTE(Val): This will directly affect our maximum refresh rate. 
#define MS_SAFETY_MARGIN 4.0f

static int32
MainLoop(program_data *pdata)
{
    //struct timespec TimeStart, TimeEnd;
    int32 time_start;
    int32 time_end;
    real64 next_frame_time;
    real64 next_video_frame_time;
    
    next_frame_time = time_start + (int32)REFRESH_RATE;
    time_start = PlatformGetTime();
    next_video_frame_time = 0;
    
    while(pdata->running)
    {
        dbg_print("time_start:\t\t\t%d\n"
                  "next_frame_time:\t\t%f\n"
                  "next_video_frame_time:\t\t%f\t\n"
                  "tick:\t\t\t\t%d\n",
                  time_start,
                  next_frame_time,
                  next_video_frame_time,
                  pdata->tick);
        
        next_frame_time += REFRESH_RATE;
        
        //clock_gettime(CLOCK_REALTIME, &TimeStart);
        
        // Get input
        // TODO(Val): Introduce some kind of timing system to see how long keys are held
        PlatformGetInput(pdata);
        
        // Process input
        int keys = pdata->input.keyboard.n;
        for(int i = 0; i < keys; i++)
        {
            key_event e = pdata->input.keyboard.events[i];
            
            switch(e.key)
            {
                case KB_F4:
                {
                    if(e.alt)
                        pdata->running = 0;
                } break;
                case KB_ESCAPE:
                {
                    pdata->running = 0;
                } break;
                case KB_UP:
                {
                    if(e.pressed)
                    {
                        pdata->volume += 0.025f;
                        
                        if(pdata->volume > 1.0f) pdata->volume = 1.0f;
                    }
                }
                case KB_DOWN:
                {
                    if(e.pressed)
                    {
                        pdata->volume -= 0.025f;
                        
                        if(pdata->volume < 0.0f) pdata->volume = 0.0f;
                    }
                }
            }
        }
        pdata->input.keyboard.n = 0;
        
        // TODO(Val): Get audio
        
        // TODO(Val): Draw UI
        
        if(pdata->file.file_ready)
        {
            if(!next_video_frame_time)
                next_video_frame_time = (real64)time_start + pdata->file.target_time;
            
            if((pdata->playing && !pdata->paused) &&
               (next_frame_time + MS_SAFETY_MARGIN >= next_video_frame_time))
            {
                //PlatformEnqueueAudio(pdata);
                if(pdata->file.has_video)
                    PlatformUpdateFrame(pdata);
                
                // increment tick value based on time passed
                next_video_frame_time += pdata->file.target_time;
                pdata->tick++;
            }
        }
        
        time_end = PlatformGetTime();
        
        int sleep_time = (int32)REFRESH_RATE - (time_end - time_start);
        if(sleep_time < 0)
            sleep_time = 0;
        dbg_print("sleep_time: %d\n", sleep_time);
        dbg_print("time_start: %d\n"
                  "time_end:   %d\n",
                  time_start, time_end);
        
        dbg_info("Sleep started.\n");
        PlatformSleep(sleep_time);
        dbg_info("Sleep ended.\n");
        
        time_start = next_frame_time;
        
        PlatformGetInput(pdata);
    }
    
    return 0;
}

static void
TogglePlayback(program_data *pdata)
{
    pdata->paused = !pdata->paused;
    PlatformPauseAudio(pdata->paused);
}

static int32
MainThread(program_data *pdata)
{
    pdata->threads.decoder_thread =
        PlatformCreateThread(DecodingThreadStart, pdata, "decoder");
    //pdata->threads.blt_thread = 
    //PlatformCreateThread(UpdateLoop, pdata, "frame_updater");
    //pdata->threads.blt_thread =
    //PlatformCreateThread(PlatformFrameUpdater, pdata, "frame_updater");
    
    MainLoop(pdata);
    
    //PlatformWaitThread(pdata->threads.blt_thread, NULL);
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
}
