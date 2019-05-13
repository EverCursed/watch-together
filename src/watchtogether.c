#include "deb-watchtogether-v2.h"
#include "watchtogether.h"
#include "platform.h"

#include "audio_queue.c"
#include "video_queue.c"
#include "decoding.c"

// TODO(Val): ffmpeg decoding/encoding
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

static int32
UpdateLoop(void *data)
{
    
}

static void
TogglePlayback(program_data *pdata)
{
    pdata->paused = !pdata->paused;
    PlatformPauseAudio(pdata->paused);
}

static int32
MainLoop(void *data)
{
    program_data *pdata = data;
    pdata->threads.decoder_thread =
        PlatformCreateThread(DecodingThreadStart, pdata, "decoder");
    //pdata->threads.blt_thread = 
    //PlatformCreateThread(UpdateLoop, pdata, "frame_updater");
    pdata->threads.blt_thread =
        PlatformCreateThread(PlatformFrameUpdater, pdata, "frame_updater");
    
    while(pdata->running)
    {
        // TODO(Val): Get input
        
        // TODO(Val): Process input
        
        // TODO(Val): Get audio
        //get_next_frame(data->Pixels, data->SoundSample);
        
        // TODO(Val): Draw UI
        PlatformSleep(100);
    }
    
    PlatformWaitThread(pdata->threads.blt_thread, NULL);
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
}
