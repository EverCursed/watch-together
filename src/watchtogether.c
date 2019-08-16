#include "watchtogether.h"
#include "platform.h"
#include "kbkeys.h"

#include "packet_queue.c"
#include "decoding.c"
#include "video.c"
#include "audio.c"
#include "playback.c"
#include "message_queue.c"

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
                if(e.pressed)
                    AddMessage0(&pdata->messages,
                                MSG_CLOSE,
                                pdata->client.current_frame_time);
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
                    AddMessage0(&pdata->messages,
                                MSG_TOGGLE_FULLSCREEN,
                                pdata->client.current_frame_time);
            } break;
            case KB_SPACE:
            {
                if(e.pressed)
                    AddMessage0(&pdata->messages,
                                MSG_PAUSE,
                                pdata->client.current_frame_time);
            } break;
        }
        pdata->input.keyboard.n = 0;
    }
}

static void
ProcessMessages(program_data *pdata)
{
    while(!MessagesEmpty(&pdata->messages) && pdata->running)
    {
        message m;
        GetMessage(&pdata->messages, &m);
        
        switch(m.msg)
        {
            case MSG_NO_MORE_MESSAGES:
            dbg_error("No message returned. Check message queue bookkeeping.\n");
            break;
            case MSG_TOGGLE_FULLSCREEN:
            PlatformToggleFullscreen(pdata);
            break;
            case MSG_START_PLAYBACK:
            
            break;
            case MSG_STOP_PLAYBACK:
            
            break;
            case MSG_PAUSE:
            TogglePlayback(pdata);
            break;
            case MSG_SEEK:
            
            break;
            case MSG_CONNECT:
            
            break;
            case MSG_WINDOW_RESIZED:
            
            break;
            case MSG_CLOSE:
            pdata->running = 0;
            break;
            default:
            dbg_warn("Unknown message in message queue: %d\n", m.msg);
        }
    }
}

static void
ProcessAudio(program_data *pdata)
{
    PlatformQueueAudio(&pdata->audio);
    
    increment_audio_times(&pdata->playback, pdata->audio.duration);
    
    PrepareAudioOutput(&pdata->audio);
    
    pdata->audio.is_ready = 0;
}

static void
ProcessVideo(program_data *pdata)
{
    PlatformUpdateFrame(pdata);
    //PrepareVideoOutput(&pdata->video);
    
    increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
    
    pdata->video.is_ready = 0;
}

static void
SkipVideoFrame(program_data *pdata)
{
    //PrepareVideoOutput(&pdata->video);
    
    increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
    
    pdata->video.is_ready = 0;
}

static int32
ProcessPlayback(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    
    bool32 need_video = 0;
    bool32 need_audio = 0;
    
    if(pdata->video.is_ready &&
       (should_display(playback, pdata->video.pts) || !playback->started_playing))
    {
        dbg_success("pdata->video.is_ready\n");
        
        ProcessVideo(pdata);
        
        need_video = 1;
    }
    else if(pdata->video.is_ready && 
            should_skip(playback, pdata->video.pts))
    {
        dbg_warn("Skipping frame.\n");
        
        SkipVideoFrame(pdata);
        
        need_video = 1;
    }
    else if(!pdata->video.is_ready)
    {
        dbg_warn("Video was not ready.\n");
    }
    else
    {
        dbg_warn("Not time to display yet.\n");
    }
    
    if(pdata->audio.is_ready &&
       (should_queue(playback) || !playback->started_playing))
    {
        ProcessAudio(pdata);
        
        need_audio = 1;
    }
    else
    {
        //dbg_error("Audio is not ready or not time to play.\n");
    }
    
    if(need_audio || need_video)
    {
        dbg_warn("Video or audio needed.\n");
        PlatformConditionSignal(&pdata->decoder.condition);
    }
    
    //update_playback_time(playback);
    
    return 0;
}

static int32
MainLoop(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    client_info *client = &pdata->client;
    
    pdata->playback.current_frame_time = &pdata->client.current_frame_time;
    pdata->playback.next_frame_time = &pdata->client.next_refresh_time;
    pdata->playback.refresh_target = &pdata->client.refresh_target;
    
    client->next_refresh_time = (PlatformGetTime() +  client->refresh_target)/1.0;
    
    // now start main loop
    while(pdata->running)
    {
        if(pdata->file.open_failed)
        {
            //pdata->file.open_failed = 0;
            
            // TODO(Val): Opening file failed.
            
            dbg_error("Opening file failed.\n");
        }
        else 
        {
            if(pdata->start_playback)
            {
                start_playback(playback, *playback->current_frame_time);
                
                pdata->start_playback = 0;
                pdata->playing = 1;
                
                ProcessVideo(pdata);
                ProcessAudio(pdata);
                
                PlatformConditionSignal(&pdata->decoder.condition);
                
                //TogglePlayback(pdata);
                dbg_warn("Playback started!\n");
            }
        }
        
        // TODO(Val): Draw UI
        
        ProcessInput(pdata);
        ProcessMessages(pdata);
        
        if(!pdata->file.open_failed)
        {
            if(pdata->playing && !pdata->paused)
            {
                ProcessPlayback(pdata);
                
                //playback->playback_time += pdata->client.refresh_target;
            }
            else
            {
                dbg_error("Not playing or paused.\n");
            }
        }
        //dbg_print("Loop time: %ld\n", playback->time_end - playback->time_start);
        
        real64 time = PlatformGetTime();
        real64 sleep_time = (client->next_refresh_time - time - 0.001);
        dbg_info("PlatformSleep(%lf)\n"
                 "\tnext_refresh_time: %lf\n"
                 "\tcurrent time: %lf\n",
                 sleep_time,
                 client->next_refresh_time,
                 time);
        PlatformSleep(sleep_time);
        
        // TODO(Val): Let's try to present at a constant interval
        
        if(!playback->started_playing)
        {
            PlatformPauseAudio(0);
            playback->started_playing = 1;
        }
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
    
    // NOTE(Val): Allocate video buffers
    // NOTE(Val): Currently this forces a conversion to YUV. 
    //            Therefore, it is not sensitive to various pixel sizes
    pdata->video.pitch = round_up_align(pdata->file.width);
    pdata->video.pitch_sup1 = round_up_align((pdata->file.width+1)/2);
    pdata->video.pitch_sup2 = round_up_align((pdata->file.width+1)/2);
    
    pdata->video.video_frame = malloc(pdata->video.pitch * pdata->file.height);
    pdata->video.video_frame_sup1 = malloc(pdata->video.pitch_sup1 * (pdata->file.height+1)/2);
    pdata->video.video_frame_sup2 = malloc(pdata->video.pitch_sup2 * (pdata->file.height+1)/2);
    
    pdata->video.width = pdata->file.width;
    pdata->video.height = pdata->file.height;
}

static bool32
DeallocateBuffers(program_data *pdata)
{
    free(pdata->audio.buffer);
    
    free(pdata->video.video_frame);
    free(pdata->video.video_frame_sup1);
    free(pdata->video.video_frame_sup2);
    
    pdata->audio.buffer = NULL;
    
    pdata->video.video_frame = NULL;
    pdata->video.video_frame_sup1 = NULL;
    pdata->video.video_frame_sup2 = NULL;
    
    return 0;
}

static bool32
InitQueues(program_data *pdata)
{
    pdata->pq_main = init_avpacket_queue(PACKET_QUEUE_SIZE);
    pdata->pq_video = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    pdata->pq_audio = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
    
    InitMessageQueue(&pdata->messages);
    
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
        //PlatformPauseAudio(1);
        
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
    dbg_error("OpenFile() failed.\n");
    pdata->file.open_failed = 1;
    return -1;
}

static bool32
CloseFile(program_data *pdata)
{
    pdata->file.file_ready = 0;
    pdata->playing = 0;
    pdata->paused = 0;
    
    PlatformConditionSignal(&pdata->decoder.condition);
    
    // TODO(Val): This will block forever, need to fix
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
    
    DeallocateBuffers(pdata);
    
    PlatformCloseAudio(pdata);
    
    return 0;
}

static int32
MainThread(program_data *pdata)
{
    // NOTE(Val): Initialize things here that will last the entire runtime of the application
    
    pdata->client.refresh_target = 1.0 / (real64)pdata->hardware.monitor_refresh_rate;
    
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
