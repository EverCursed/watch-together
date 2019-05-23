//#include "deb-watchtogether-v2.h"
#include "watchtogether.h"
#include "platform.h"

#include "audio_queue.c"
#include "video_queue.c"
#include "decoding.c"

#include <time.h>

// TODO(Val): ffmpeg decoding/encoding
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

static int32
UpdateLoop(program_data  *pdata)
{
    struct timespec TimeStart, TimeEnd;
    
    while(pdata->running)
    {
        clock_gettime(CLOCK_REALTIME, &TimeStart);
        
        // TODO(Val): Get input
        PlatformGetInput(pdata);
        
        // TODO(Val): Process input
        
        // TODO(Val): Get audio
        //get_next_frame(data->Pixels, data->SoundSample);
        
        // TODO(Val): Draw UI
        
        PlatformFrameUpdater(pdata);
        
        clock_gettime(CLOCK_REALTIME, &TimeEnd);
        
        // TODO(Val): Rewrite the sleep for frame updater, maybe use SDL stuff somehow?
        struct timespec TimeDifference = time_diff(TimeEnd, TimeStart);
        //dbg_print("TimeDiff: tv_sec = %ld\ttv_nsec = %ld\n", TimeDifference.tv_sec, TimeDifference.tv_nsec);
        
        struct timespec target = {};
        target.tv_sec = (uint64)(pdata->file.target_time / 1000.0f);
        target.tv_nsec = (uint64)(pdata->file.target_time * 1000000.0f);
        
        //dbg_print("Target: tv_sec = %ld\ttv_nsec = %ld\n", target.tv_sec, target.tv_nsec);
        
        struct timespec SleepDuration = time_diff(target, TimeDifference);
        
        //dbg_print("Nanosleep: tv_sec = %ld\ttv_nsec = %ld\n", SleepDuration.tv_sec, SleepDuration.tv_nsec);
        nanosleep(&SleepDuration, NULL);
    }
}

static void
TogglePlayback(program_data *pdata)
{
    pdata->paused = !pdata->paused;
    PlatformPauseAudio(pdata->paused);
}

static int32
MainLoop(program_data *pdata)
{
    pdata->threads.decoder_thread =
        PlatformCreateThread(DecodingThreadStart, pdata, "decoder");
    //pdata->threads.blt_thread = 
    //PlatformCreateThread(UpdateLoop, pdata, "frame_updater");
    //pdata->threads.blt_thread =
    //PlatformCreateThread(PlatformFrameUpdater, pdata, "frame_updater");
    
    UpdateLoop(pdata);
    
    //PlatformWaitThread(pdata->threads.blt_thread, NULL);
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
}
