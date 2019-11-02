#include "common/custom_malloc.h"

#include "watchtogether.h"
#include "message_queue.h"
#include "audio.h"
#include "utils/timing.h"
#include "network.h"
#include "media_processing.h"
#include "time.h"
//#include "platform.h"
//#include "kbkeys.h"
//#include "utils.h"
//#include "audio.h"
//#include "playback.h"
//#include "packet_queue.h"

//#include <time.h>
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

// TODO(Val): Test a variety of these, and see how long it's possible to go
// NOTE(Val): This will directly affect our maximum refresh rate. 
#define MS_SAFETY_MARGIN 2.0

#define PACKET_QUEUE_SIZE 60

#define VOLUME_STEP 0.025f

static void
TogglePlayback(program_data *pdata)
{
    if(!pdata->paused)
    {
        pdata->playback.pause_started = *pdata->playback.current_frame_time;
    }
    else
    {
        real64 time = *pdata->playback.current_frame_time;
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
    StartTimer("Input processing");
    int keys = pdata->input.keyboard.n;
    for(int i = 0; i < keys && pdata->running; i++)
    {
        StartTimer("Event");
        key_event e = pdata->input.keyboard.events[i];
        
        switch(e.key)
        {
            case KB_F4:
            {
                if(e.alt && e.pressed)
                    AddMessage0(&pdata->messages,
                                MSG_CLOSE,
                                pdata->client.current_frame_time);
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
                    arg a = { .f = VOLUME_STEP };
                    AddMessage1(&pdata->messages,
                                MSG_VOLUME_CHANGE,
                                a,
                                pdata->client.current_frame_time);
                }
            } break;
            case KB_DOWN:
            {
                if(e.pressed)
                {
                    arg a = { .f = -VOLUME_STEP };
                    AddMessage1(&pdata->messages,
                                MSG_VOLUME_CHANGE,
                                a,
                                pdata->client.current_frame_time);
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
        EndTimer();
    }
    EndTimer();
    
    return 0;
}

static void
ProcessMessages(program_data *pdata)
{
    StartTimer("ProcessMessages inner");
    
    while(!MessagesEmpty(&pdata->messages) && pdata->running)
    {
        StartTimer("Message");
        
        message m;
        GetApplicationMessage(&pdata->messages, &m);
        switch(m.msg)
        {
            case MSG_NO_MORE_MESSAGES:
            dbg_error("No message returned. Check message queue bookkeeping.\n");
            break;
            case MSG_TOGGLE_FULLSCREEN:
            {
                PlatformToggleFullscreen(pdata);
                PlatformFlipBuffers(pdata);
            } break;
            case MSG_START_PLAYBACK:
            {
                
            } break;
            case MSG_STOP_PLAYBACK:
            {
                
            } break;
            case MSG_PAUSE:
            {
                TogglePlayback(pdata);
            } break;
            case MSG_SEEK:
            {
                
            } break;
            case MSG_CONNECT:
            {
                
            } break;
            case MSG_WINDOW_RESIZED:
            {
                
            } break;
            case MSG_CLOSE:
            {
                pdata->running = 0;
            } break;
            case MSG_VOLUME_CHANGE:
            {
                if(m.args[0].f < 0.0)
                {
                    if(pdata->volume > 0.0) 
                    {
                        pdata->volume += m.args[0].f;
                        if(pdata->volume < 0)
                            pdata->volume = 0.0;
                    }
                }
                else if(m.args[0].f > 0.0)
                {
                    if(pdata->volume < 1.0) 
                    {
                        pdata->volume += m.args[0].f;
                        if(pdata->volume > 1)
                            pdata->volume = 1.0;
                    }
                }
            } break;
            default:
            {
                dbg_warn("Unknown message in message queue: %d\n", m.msg);
            }
        }
        
        EndTimer();
    }
    
    EndTimer();
}

static void
ProcessAudio(program_data *pdata)
{
    StartTimer("ProcessAudio()");
    
    PlatformLockMutex(&pdata->audio.mutex);
    
    PlatformQueueAudio(&pdata->audio);
    increment_audio_times(&pdata->playback, pdata->audio.duration);
    PrepareAudioOutput(&pdata->audio);
    
    pdata->audio.is_ready = 0;
    
    PlatformUnlockMutex(&pdata->audio.mutex);
    
    EndTimer();
}

static void
ProcessVideo(program_data *pdata)
{
    PlatformUpdateVideoFrame(pdata);
    //ClearVideoOutput(&pdata->video);
    
    increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
    
    pdata->video.is_ready = 0;
}

static void
SkipVideoFrame(program_data *pdata)
{
    //ClearVideoOutput(&pdata->video);
    
    increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
    
    pdata->video.is_ready = 0;
}

static int32
ProcessPlayback(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    
    bool32 need_video = 0;
    bool32 need_audio = 0;
    
    if(pdata->file.has_video)
    {
        StartTimer("Processing video");
        if(pdata->video.is_ready &&
           (should_display(playback, pdata->video.pts) || !playback->started_playing))
        {
            dbg_success("pdata->video.is_ready\n");
            
            StartTimer("ProcessVideo()");
            ProcessVideo(pdata);
            EndTimer();
            
            need_video = 1;
        }
        else if(pdata->video.is_ready && 
                should_skip(playback, pdata->video.pts))
        {
            dbg_warn("Skipping frame.\n");
            
            StartTimer("SkipVideoFrame()");
            SkipVideoFrame(pdata);
            EndTimer();
            
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
        EndTimer();
    }
    
    if(pdata->file.has_audio)
    {
        StartTimer("Processing audio");
        if(pdata->audio.is_ready &&
           (should_queue(playback) || !playback->started_playing))
        {
            dbg_error("Processing audio.\n");
            ProcessAudio(pdata);
            
            need_audio = 1;
        }
        //else if(pdata->audio.is_ready &&
        //(should_skip(playback, playback->audio_total_queued)))
        //{
        //dbg_error("Audio is not ready or not time to play.\n");
        //pdata->audio.is_ready = 0;
        //need_audio = 1;
        //}
        else if(pdata->audio.is_ready)
        {
            dbg_error("Just waiting as it's not yet time to queue.\n");
        }
        else
        {
            dbg_error("Audio is not ready.\n");
        }
        EndTimer();
    }
    
    
    if(need_audio || need_video)
    {
        dbg_warn("Video or audio needed.\n");
        PlatformConditionSignal(&pdata->decoder.condition);
    }
    //update_playback_time(playback);
    
    RETURN(SUCCESS);
}

int32
ProcessNetwork(program_data *pdata)
{
    if(pdata->is_host && !pdata->connected)
    {
        if(AcceptConnection() == CONNECTED)
        {
            pdata->connected = 1;
            
            //PreSendPackets();
        }
    }
    else if(pdata->is_host && pdata->connected)
    {
        
    }
    else if(pdata->is_partner)
    {
        
    }
    
    RETURN(SUCCESS);
}

int32
InputLoopThread(void *arg)
{
    InitializeTimingSystem("input");
    
    program_data *pdata = arg;
    
    while(pdata->running)
    {
        StartTimer("ProcessInput()");
        ProcessInput(pdata);
        EndTimer();
    }
    
    FinishTiming();
    
    RETURN(SUCCESS);
}

static void
StartPlayback(program_data *pdata)
{
    StartTimer("Starting Playback");
    start_playback(&pdata->playback, *pdata->playback.current_frame_time + 0.3);
    
    pdata->start_playback = 0;
    pdata->playing = 1;
    
    //ProcessVideo(pdata);
    //ProcessAudio(pdata);
    
    PlatformConditionSignal(&pdata->decoder.condition);
    
    //TogglePlayback(pdata);
    dbg_warn("Playback started!\n");
    EndTimer();
}

int32
MainLoopThread(void *arg)
{
    InitializeTimingSystem("main");
    
    program_data *pdata = arg;
    playback_data *playback = &pdata->playback;
    client_info *client = &pdata->client;
    
    client->start_time = PlatformGetTime();
    client->next_refresh_time = (client->start_time + client->refresh_target);
    
    // now start main loop
    
    if(pdata->is_host)
    {
        StartServer();
    }
    
    if(pdata->is_partner)
    {
        if(!ConnectToIP(pdata->server_ip))
        {
            pdata->connected = 1;
        }
        else
        {
            pdata->running = 0;
            RETURN(CONNECTION_FAILED);
        }
    }
    
    StartTimer("MainLoop()");
    while(pdata->running)
    {
        StartTimer("Loop");
        
        assert_memory_bounds();
        
        StartTimer("ProcessMessages()");
        ProcessMessages(pdata);
        EndTimer();
        
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
                StartPlayback(pdata);
            }
        }
        
        if(!pdata->file.open_failed)
        {
            if(pdata->playing && !pdata->paused)
            {
                StartTimer("ProcessPlayback()");
                ProcessPlayback(pdata);
                EndTimer();
                //playback->playback_time += pdata->client.refresh_target;
            }
            else
            {
                dbg_error("Not playing or paused.\n");
            }
        }
        //dbg_print("Loop time: %ld\n", playback->time_end - playback->time_start);
        
        real64 time = PlatformGetTime();
        real64 sleep_time = (client->next_refresh_time - time - 0.002);
        dbg_info("PlatformSleep(%lf)\n"
                 "\tnext_refresh_time: %lf\n"
                 "\tcurrent time: %lf\n",
                 sleep_time,
                 client->next_refresh_time,
                 time);
        
        StartTimer("Sleep()");
        //PlatformSleep(sleep_time);
        WaitUntil(client->next_refresh_time, 0.002);
        EndTimer();
        
        StartTimer("PlatformFlipBuffers()");
        if(!playback->started_playing)
        {
            PlatformPauseAudio(0);
            playback->started_playing = 1;
        }
        PlatformUpdateFrame(pdata);
        PlatformFlipBuffers(pdata);
        EndTimer();
        
        client->current_frame_time = PlatformGetTime();
        client->next_refresh_time += client->refresh_target;
        
        EndTimer();
    }
    EndTimer();
    
    FinishTiming();
    RETURN(SUCCESS);
}

static int32
AllocateBuffers(program_data *pdata)
{
    // NOTE(Val): Allocate audio buffer
    int32 bytes_per_sample = pdata->file.bytes_per_sample;
    int32 sample_rate = pdata->file.sample_rate;
    int32 channels = pdata->file.channels;
    int32 seconds = 1;
    
    pdata->audio.max_buffer_size = bytes_per_sample*sample_rate*channels*seconds;
    pdata->audio.buffer = custom_malloc(pdata->audio.max_buffer_size);
    
    // NOTE(Val): Allocate video buffers
    pdata->video.pitch = round_up_align(pdata->file.width * 4);
    //pdata->video.pitch_sup1 = round_up_align((pdata->file.width+1)/2);
    //pdata->video.pitch_sup2 = round_up_align((pdata->file.width+1)/2);
    
    pdata->video.video_frame = custom_malloc(pdata->video.pitch * pdata->file.height);
    //pdata->video.video_frame_sup1 = malloc(pdata->video.pitch_sup1 * (pdata->file.height+1)/2);
    //pdata->video.video_frame_sup2 = malloc(pdata->video.pitch_sup2 * (pdata->file.height+1)/2);
    
    pdata->video.width = pdata->file.width;
    pdata->video.height = pdata->file.height;
    
    RETURN(SUCCESS);
}

static int32
DeallocateBuffers(program_data *pdata)
{
    free(pdata->audio.buffer);
    
    pdata->audio.buffer = NULL;
    
    free(pdata->video.video_frame);
    //free(pdata->video.video_frame_sup1);
    //free(pdata->video.video_frame_sup2);
    
    
    pdata->video.video_frame = NULL;
    //pdata->video.video_frame_sup1 = NULL;
    //pdata->video.video_frame_sup2 = NULL;
    
    RETURN(SUCCESS);
}

static int32
InitializeApplication(program_data *pdata)
{
    InitMessageQueue(&pdata->messages);
    
    RETURN(SUCCESS);
}

static int32
InitQueues(program_data *pdata)
{
    pdata->pq_main = init_avpacket_queue(PACKET_QUEUE_SIZE);
    
    if(pdata->file.has_video)
    {
        pdata->pq_video = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
        if(pdata->pq_video == NULL)
            RETURN(NO_MEMORY);
    }
    
    if(pdata->file.has_audio)
    {
        pdata->pq_audio = init_avpacket_queue(PACKET_QUEUE_SIZE/2);
        if(pdata->pq_audio == NULL)
            RETURN(NO_MEMORY);
    }
    
    RETURN(SUCCESS);
}

static int32
TerminateQueues(program_data *pdata)
{
    close_avpacket_queue(pdata->pq_audio);
    close_avpacket_queue(pdata->pq_video);
    close_avpacket_queue(pdata->pq_main);
    
    RETURN(SUCCESS);
}

static int32
FileOpen(program_data *pdata)
{
    pdata->decoder.condition = PlatformCreateConditionVar();
    
    if(!MediaOpen(&pdata->file, &pdata->decoder, &pdata->encoder))
    {
        InitQueues(pdata);
        
        if(pdata->file.has_audio)
        {
            pdata->audio.mutex = PlatformCreateMutex();
            PlatformInitAudio(pdata);
        }
        if(pdata->file.has_video)
        {
            PlatformInitVideo(pdata);
        }
        //PlatformPauseAudio(1);
        
        AllocateBuffers(pdata);
        
        pdata->threads.decoder_thread =
            PlatformCreateThread(MediaThreadStart, pdata, "decoder");
        
        pdata->file.file_ready = 1;
        pdata->playing = 0;
        pdata->paused = 0;
        
        RETURN(SUCCESS);
    }
    else
    {
        pdata->file.open_failed = 1;
        RETURN(UNKNOWN_ERROR);
    }
}

static bool32
FileClose(program_data *pdata)
{
    pdata->file.file_ready = 0;
    pdata->playing = 0;
    pdata->paused = 0;
    
    PlatformConditionSignal(&pdata->decoder.condition);
    
    MediaClose(&pdata->file, &pdata->decoder, &pdata->encoder);
    PlatformConditionDestroy(&pdata->decoder.condition);
    
    // TODO(Val): This will block forever, need to fix
    PlatformWaitThread(pdata->threads.decoder_thread, NULL);
    
    DeallocateBuffers(pdata);
    
    PlatformCloseAudio(pdata);
    
    TerminateQueues(pdata);
    
    RETURN(SUCCESS);
}

int32
MainThread(program_data *pdata)
{
    InitializeApplication(pdata);
    // NOTE(Val): Initialize things here that will last the entire runtime of the application
    
    pdata->client.refresh_target = 1.0 / (real64)pdata->hardware.monitor_refresh_rate;
    //pdata->client.
    
    pdata->playback.current_frame_time = &pdata->client.current_frame_time;
    pdata->playback.next_frame_time = &pdata->client.next_refresh_time;
    pdata->playback.refresh_target = &pdata->client.refresh_target;
    
    dbg_info("Client refresh target time set to %lfs.\n", pdata->client.refresh_target);
    
    //pdata->threads.input_thread = PlatformCreateThread(InputLoopThread, pdata, "input_thread");
    pdata->threads.main_thread = PlatformCreateThread(MainLoopThread, pdata, "main_thread");
    
    if(!FileOpen(pdata))
    {
        InputLoopThread(pdata);
        //MainLoopThread(pdata);
        FileClose(pdata);
    }
    else
    {
        dbg_error("File isn't found.\n");
        pdata->running = 0;
    }
    
    pdata->running = 0;
    return 0;
}
