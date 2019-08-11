#include "watchtogether.h"
#include "platform.h"
#include "kbkeys.h"

#include "packet_queue.c"
#include "decoding.c"
#include "video.c"
#include "audio.c"
#include "playback.c"

//#include <time.h>
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

// TODO(Val): Test a variety of these, and see how long it's possible to go
// NOTE(Val): This will directly affect our maximum refresh rate. 
#define MS_SAFETY_MARGIN 2.0f

#define PACKET_QUEUE_SIZE 60

static void
TogglePlayback(program_data *pdata)
{
    if(!pdata->paused)
    {
        pdata->playback.pause_started = PlatformGetTime();
    }
    else
    {
        real64 time = PlatformGetTime();
        pdata->playback.aggregated_pause_time += (time - pdata->playback.pause_started);
    }
    
    pdata->paused = !pdata->paused;
    PlatformPauseAudio(pdata->paused);
}

static int32
ProcessInput(program_data *pdata)
{
    // Get input
    // TODO(Val): Introduce some kind of timing system to see how long keys are held
    //if(pdata->running)
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
            } break;
            case KB_DOWN:
            {
                if(e.pressed)
                {
                    pdata->volume -= 0.025f;
                    
                    if(pdata->volume < 0.0f) pdata->volume = 0.0f;
                }
            } break;
            case KB_ENTER:
            {
                if(e.pressed && e.alt)
                {
                    PlatformToggleFullscreen(pdata);
                }
            } break;
            case KB_SPACE:
            {
                if(e.pressed)
                    TogglePlayback(pdata);
            } break;
        }
    }
    pdata->input.keyboard.n = 0;
    
}

static int32
ProcessPlayback(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    
    bool32 need_video = 0;
    bool32 need_audio = 0;
    
    if(pdata->video.is_ready)
    {
        if(should_display(playback, pdata->video.pts))
        {
            dbg_success("pdata->video.is_ready\n");
            PlatformUpdateFrame(pdata);
            PrepareVideoOutput(&pdata->video);
            
            pdata->video.is_ready = 0;
            
            //increment_video_times(playback, av_q2d(pdata->decoder.video_time_base));
            
            need_video = 1;
            //need_flip = 1;
        }
        else if(should_skip(playback, pdata->video.pts))
        {
            dbg_warn("Skipping frame.\n");
            
            PrepareVideoOutput(&pdata->video);
            pdata->video.is_ready = 0;
        }
        else
        {
            dbg_warn("Not time to display yet.\n");
            dbg_print("video_pts: %lf\n", pdata->video.pts);
        }
    }
    else
    {
        dbg_error("Video was not ready.\n");
        
        // TODO(Val): skip this frame
    }
    // TODO(Val): Delay playing this until we can't delay any more.
    
    if(pdata->audio.is_ready)
    {
        if(should_queue(playback))
        {
            PlatformQueueAudio(&pdata->audio);
            
            increment_audio_times(playback, pdata->audio.duration);
            
            PrepareAudioOutput(&pdata->audio);
            
            //pdata->audio.required_duration = 
            need_audio = 1;
        }
        else
        {
            dbg_error("should_queue(playback) failed.\n");
        }
    }
    else
    {
        dbg_error("Audio is not ready.\n");
    }
    
    if(need_audio || need_video)
    {
        //dbg_info("Video or audio needed.\n");
        PlatformConditionSignal(&pdata->decoder.condition);
    }
    
    update_playback_time(playback);
    
    return 0;
}

static int32
MainLoop(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    client_info *client = &pdata->client;
    
    client->next_refresh_time = (PlatformGetTime() +  client->refresh_target)/1000.0;
    
    // times needed for video playback
    
    dbg_print("audio time_base: %d/%d\n", pdata->decoder.audio_time_base.num, pdata->decoder.audio_time_base.den);
    
    // now start main loop
    while(pdata->running)
    {
        if(pdata->file.open_failed)
        {
            pdata->file.open_failed = 0;
            
            // TODO(Val): Opening file failed.
            
            dbg_error("Opening file failed.\n");
        }
        
        if(pdata->start_playback)
        {
            start_playback(playback, PlatformGetTime());
            
            pdata->start_playback = 0;
            pdata->playing = 1;
            
            //TogglePlayback(pdata);
        }
        
        // TODO(Val): Draw UI
        
        ProcessInput(pdata);
        
        if(pdata->playing && !pdata->paused)
        {
            ProcessPlayback(pdata);
            
            playback->playback_time += pdata->client.refresh_target;
        }
        //dbg_print("Loop time: %ld\n", playback->time_end - playback->time_start);
        //dbg_info("PlatformSleep(%lf)\n", playback->next_frame_time - PlatformGetTime());
        
        PlatformSleep(client->next_refresh_time - PlatformGetTime() - 1);
        
        // TODO(Val): Let's try to present at a constant interval
        PlatformFlipBuffers(pdata);
        
        client->current_frame_time = client->next_refresh_time;
        client->next_refresh_time += client->refresh_target;
    }
    
    return 0;
}

static bool32
AllocateBuffers(program_data *pdata)
{
    // NOTE(Val): Allocate audio buffer
    int32 bytes_per_sample = pdata->file.bytes_per_sample;
    int32 sample_rate = pdata->file.sample_rate;
    int32 channels = pdata->file.channels;
    int32 seconds = 1;
    
    pdata->audio.buffer = malloc(bytes_per_sample*sample_rate*channels*seconds);
    
    // TODO(Val): Allocate video buffers
}

static bool32
DeallocateBuffers(program_data *pdata)
{
    free(pdata->audio.buffer);
    
    return 0;
}

static bool32
InitQueues(program_data *pdata)
{
    pdata->pq_main = init_avpacket_queue(PACKET_QUEUE_SIZE);
    pdata->pq_video = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    pdata->pq_audio = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    
    return 0;
}

static bool32
TerminateQueues(program_data *pdata)
{
    close_avpacket_queue(pdata->pq_audio);
    close_avpacket_queue(pdata->pq_video);
    close_avpacket_queue(pdata->pq_main);
    
    return 0;
}

static bool32
OpenFile(program_data *pdata)
{
    if(!DecodingFileOpen(&pdata->file, &pdata->decoder))
    {
        PlatformInitAudio(pdata);
        PlatformInitVideo(pdata);
        
        AllocateBuffers(pdata);
        
        pdata->threads.decoder_thread =
            PlatformCreateThread(DecodingThreadStart, pdata, "decoder");
        
        pdata->file.file_ready = 1;
        pdata->playing = 0;
        pdata->paused = 0;
        
        return 0;
    }
    else goto open_failed;
    
    open_failed:
    return -1;
}

static bool32
CloseFile(program_data *pdata)
{
    pdata->file.file_ready = 0;
    pdata->playing = 0;
    pdata->paused = 0;
    
    PlatformConditionSignal(&pdata->decoder.condition);
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
    
    DeallocateBuffers(pdata);
    
    PlatformCloseAudio(pdata);
    
    return 0;
}

static int32
MainThread(program_data *pdata)
{
    // NOTE(Val): Initialize things here that will last the entire runtime of the application
    
    pdata->client.refresh_target = 1000.0 / (real64)pdata->hardware.monitor_refresh_rate;
    pdata->playback.current_frame_time = &pdata->client.current_frame_time;
    pdata->playback.next_frame_time = &pdata->client.next_refresh_time;
    pdata->playback.refresh_target = &pdata->client.refresh_target;
    
    dbg_info("Client refresh target time set to %lfs.\n", pdata->client.refresh_target);
    
    // NOTE(Val): Temporarily here
    InitQueues(pdata);
    
    if(!OpenFile(pdata))
    {
        MainLoop(pdata);
        
        // NOTE(Val): Temporarily here
        CloseFile(pdata);
    }
    else
    {
        dbg_error("File isn't found.\n");
        pdata->running = 0;
    }
    
    TerminateQueues(pdata);
    
    return 0;
}
