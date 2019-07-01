#include "watchtogether.h"
#include "platform.h"
#include "kbkeys.h"

#include "packet_queue.c"
//#include "audio_queue.c"
//#include "video_queue.c"
#include "decoding.c"

//#include <time.h>
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

/*
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
*/
// TODO(Val): Find a way to get the refresh rate of the screen, for now this is a define
#define REFRESH_RATE     16.666666666666f
// TODO(Val): Test a variety of these, and see how long it's possible to go
// NOTE(Val): This will directly affect our maximum refresh rate. 
#define MS_SAFETY_MARGIN 2.0f

static real32
get_timestamp(int64 time, AVRational time_base)
{
    dbg_print("time: %ld\t\ttime_base: %d/%d\n", time, time_base.num, time_base.den);
    
    return (real32)time*(real32)time_base.num/(real32)time_base.den;
}

static int32
MainLoop(program_data *pdata)
{
    // times needed for application framerate.
    int32 time_start;
    int32 time_end;
    real64 current_frame_time;
    real64 next_frame_time;
    
    time_start = PlatformGetTime();
    current_frame_time = time_start;
    next_frame_time = current_frame_time + REFRESH_RATE;
    
    // times needed for video playback
    real64 playback_start;
    real64 current_video_frame_time;
    real64 next_video_frame_time;
    real64 aggregated_pause_time;
    
    dbg_print("audio time_base: %d/%d\n", pdata->decoder.audio_time_base.num, pdata->decoder.audio_time_base.den);
    
    // now start main loop
    while(pdata->running)
    {
        dbg_print("time_start:\t\t\t%d\n"
                  "current_frame_time:\t\t%f\n"
                  "next_frame_time:\t\t%f\n"
                  "current_video_frame_time:\t%f\n"
                  "next_video_frame_time:\t\t%f\n"
                  "tick:\t\t\t\t%d\n",
                  time_start,
                  current_frame_time, 
                  next_frame_time,
                  current_video_frame_time,
                  next_video_frame_time,
                  pdata->tick);
        
        if(pdata->start_playback)
        {
            playback_start = current_frame_time;
            current_video_frame_time = current_frame_time;
            next_video_frame_time = current_video_frame_time + 1000.0f*av_q2d(pdata->decoder.video_time_base);
            aggregated_pause_time = 0.0f;
            
            pdata->start_playback = 0;
            pdata->playing = 1;
        }
        
        // Get input
        // TODO(Val): Introduce some kind of timing system to see how long keys are held
        if(pdata->running)
            PlatformGetInput(pdata);
        
        // Process input
        int keys = pdata->input.keyboard.n;
        for(int i = 0; i < keys && pdata->running; i++)
        {
            key_event e = pdata->input.keyboard.events[i];
            
            switch(e.key)
            {
                case KB_F4:
                {
                    if(e.alt)
                    {
                        pdata->running = 0;
                    }
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
        
        
        // TODO(Val): Draw UI
        
        if(pdata->playing)
        {
            if(next_video_frame_time <= next_frame_time - MS_SAFETY_MARGIN)
            {
                dbg_info("next_video_frame_time <= next_frame_time - MS_SAFETY_MARGIN\n");
                if(pdata->video.is_ready)
                {
                    dbg_success("pdata->video.is_ready\n");
                    // TODO(Val): update video frame;
                    PlatformUpdateFrame(pdata);
                    
                    free(pdata->video.video_frame);
                    free(pdata->video.video_frame_sup1);
                    free(pdata->video.video_frame_sup2);
                    
                    current_video_frame_time = next_video_frame_time;
                    next_video_frame_time += 1000.0f*av_q2d(pdata->decoder.video_time_base);
                }
                else
                {
                    dbg_warn("Video is not ready.\n");
                    // TODO(Val): skip this frame
                }
            }
        }
        
        time_end = PlatformGetTime();
        
        PlatformSleep(next_frame_time - PlatformGetTime() - MS_SAFETY_MARGIN);
        
        dbg_print("Loop time: %d\n", time_end - time_start);
        time_start = time_end;
        
        current_frame_time = next_frame_time;
        next_frame_time += REFRESH_RATE;
    }
    
    return 0;
}

static void
TogglePlayback(program_data *pdata)
{
    pdata->paused = !pdata->paused;
    PlatformPauseAudio(pdata->paused);
}

#define PACKET_QUEUE_SIZE 120
static int32
MainThread(program_data *pdata)
{
    pdata->pq_main = init_avpacket_queue(PACKET_QUEUE_SIZE);
    pdata->pq_video = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    pdata->pq_audio = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    
    file_open(&pdata->file, &pdata->decoder);
    PlatformInitAudio(pdata);
    PlatformInitVideo(pdata);
    
    pdata->file.file_ready = 1;
    pdata->playing = 0;
    pdata->paused = 0;
    
    
    pdata->threads.decoder_thread =
        PlatformCreateThread(DecodingThreadStart, pdata, "decoder");
    
    MainLoop(pdata);
    
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
    
    PlatformCloseAudio(pdata);
    
    
    close_avpacket_queue(pdata->pq_audio);
    close_avpacket_queue(pdata->pq_video);
    close_avpacket_queue(pdata->pq_main);
}
