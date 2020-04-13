/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

The main runtime module that handles most of the application functionality.
*/

#include "watchtogether.h"
#include "attributes.h"

#include "common/vector.c"
#include "common/custom_malloc.c"
#include "logging.h"

#include "message_queue.c"
#include "audio.c"
#include "video.c"
#include "utils/timing.c"
#include "network.c"
#include "media_processing.c"
#include "time.c"
#include "avframe_pts_ordered_queue.c"
#include "streaming.c"
#include "decoding.c"
//#include "encoding.c"
#include "packet_queue.c"
#include "playback.c"
#include "ui.c"
#include "platform.c"
#include "logging.c"


// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

#define MS_SAFETY_MARGIN 2.0
#define PACKET_QUEUE_SIZE 60

internal void
LocalPause(program_data *pdata)
{
    wlog(LOG_INFO, "Pausing");
    
    playback_start_pause(&pdata->playback);
    
    SetPlaybackPaused(pdata);
}

internal void
LocalUnpause(program_data *pdata)
{
    wlog(LOG_INFO, "Unpausing");
    
    playback_end_pause(&pdata->playback);
    
    SetPlaybackUnpaused(pdata);
}

internal void
LocalTogglePlayback(program_data *pdata)
{
    wlog(LOG_INFO, "Toggling paused state");
    
    b32 paused = PlaybackPaused(pdata);
    
    if(paused)
        LocalUnpause(pdata);
    else
        LocalPause(pdata);
    
    PlatformPauseAudio(PlaybackPaused(pdata));
}

internal void
TogglePlayback(program_data *pdata)
{
    wlog(LOG_INFO, "Toggling paused state and sending network message");
    
    if(Connected(pdata))
    {
        if(PlaybackPaused(pdata))
            SendPlayMessage();
        else
            SendPauseMessage();
    }
    LocalTogglePlayback(pdata);
}

internal void
CloseMainMenu(void *ptr)
{
    program_data *pdata = ptr;
    
    PopMenuScreen(pdata->menu);
}

internal void
MarkExit(void *ptr)
{
    program_data *pdata = ptr;
    pdata->running = 0;
}

internal void
ShowOptionsScreen(void *ptr)
{
    program_data *pdata = ptr;
    PushMenuScreen(pdata->menu, pdata->menu->options_screen);
}

internal i32
ProcessInput(program_data *pdata)
{
    // Get input
    // TODO(Val): Introduce some kind of timing system to see how long keys are held
    //if(pdata->running)
    // Process input
    wlog(LOG_TRACE, "ProcessInput(): Starting input processing");
    
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
                                MSG_TOGGLE_MENU,
                                pdata->client.current_frame_time);
            } break;
            case KB_UP:
            {
                if(e.pressed)
                {
                    //arg a = { .f = VOLUME_STEP };
                    //AddMessage1(&pdata->messages,
                    //MSG_VOLUME_CHANGE,
                    //a,
                    //pdata->client.current_frame_time);
                }
            } break;
            case KB_DOWN:
            {
                if(e.pressed)
                {
                    //arg a = { .f = -VOLUME_STEP };
                    //AddMessage1(&pdata->messages,
                    //MSG_VOLUME_CHANGE,
                    //a,
                    //pdata->client.current_frame_time);
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

internal i32
ProcessMouse(program_data *pdata)
{
    Menu *m = pdata->menu;
    mouse_info *mouse = &pdata->input.mouse;
    
    if(mouse->left_button_was_pressed && !mouse->left_button_is_pressed)
    {
        int x = mouse->x;
        int y = mouse->y;
        int screen_x = pdata->client.output_width;
        int screen_y = pdata->client.output_height;
        
        func function = MenuGetClickedButtonAction(m, x, y, screen_x, screen_y);
        
        if(function)
        {
            function(pdata);
        }
    }
    
    return 0;
}

internal i32
AllocateBuffers(program_data *pdata)
{
    wlog(LOG_INFO, "AllocateBuffers(): Allocating audio and video buffers");
    
    // NOTE(Val): Allocate audio buffer
    i32 bytes_per_sample = pdata->file.bytes_per_sample;
    i32 sample_rate = pdata->file.sample_rate;
    i32 channels = pdata->file.channels;
    
    i32 seconds = 1;
    
    pdata->audio.max_buffer_size = bytes_per_sample*sample_rate*channels*seconds;
    
    // NOTE(Val): Allocate video buffers
    pdata->video.pitch = round_up_align(pdata->file.width * 4);
    pdata->video.video_frame = custom_malloc(pdata->video.pitch * pdata->file.height);
    pdata->video.width = pdata->file.width;
    pdata->video.height = pdata->file.height;
    
    RETURN(SUCCESS);
}

internal i32
DeallocateBuffers(program_data *pdata)
{
    wlog(LOG_INFO, "DeallocateBuffers(): Deallocating audio and video buffers");
    
    free(pdata->video.video_frame);
    
    pdata->video.video_frame = NULL;
    
    RETURN(SUCCESS);
}

internal void inline
InitializePointers(program_data *pdata)
{
    pdata->playback.current_frame_time = &pdata->client.current_frame_time;
    pdata->playback.next_frame_time = &pdata->client.next_refresh_time;
    pdata->playback.refresh_target = &pdata->client.refresh_target;
}

internal void
InitializeMenus(program_data *pdata)
{
    Menu *m = CreateMenuMenu();
    if(!m)
        return;
    
    // TODO(Val): maybe store the pointers to these somewhere else as well?
    MenuScreen *main_menu_screen = CreateMenuScreen(m);
    MenuScreen *options_screen   = CreateMenuScreen(m);
    MenuScreen *debug_screen     = CreateMenuScreen(m);
    
    // TODO(Val): Add functions to these
    CreateMenuButton(main_menu_screen, 0.375f, 0.125f, 0.25f, 0.05f, "Close Menu", &CloseMainMenu);
    CreateMenuButton(main_menu_screen, 0.375f, 0.2f, 0.25f, 0.05f, "Options", &ShowOptionsScreen);
    CreateMenuButton(main_menu_screen, 0.375f, 0.275f, 0.25f, 0.05f, "Turn On Debug Screen", NULL);
    CreateMenuButton(main_menu_screen, 0.375f, 0.825f, 0.25f, 0.05f, "Quit", &MarkExit);
    
#ifdef DEBUG
    CreateMenuTextBox(main_menu_screen, 0.0f, 0.0f, 0.15f, 0.05f, 1.0f, "DEBUG MODE");
#endif
    
    CreateMenuTextBox(debug_screen, 0.0f, 0.0f, 0.5f, 0.01f, 1.0f, "DEBUG SCREEN");
    
    CreateMenuTextBox(options_screen, 0.40f, 0.45f, 0.20f, 0.10f, 1.0f, "Not yet implemented.");
    
    pdata->menu = m;
    m->main_menu_screen = main_menu_screen;
    m->options_screen = options_screen;
    m->debug_screen = debug_screen;
}

internal i32
InitializeApplication(program_data *pdata)
{
    wlog(LOG_INFO, "InitializeApplication(): Initializing application data");
    
    InitMessageQueue(&pdata->messages);
    InitializePointers(pdata);
    
    Client_SetRefreshTime(&pdata->client, 1.0 / (f64)pdata->hardware.monitor_refresh_rate);
    
    InitializeMenus(pdata);
    if(!pdata->menu)
        RETURN(UNKNOWN_ERROR);
    
    RETURN(SUCCESS);
}

internal i32
FileOpen_LoadData(program_data *pdata)
{
    int ret = MediaOpen(&pdata->file, &pdata->decoder, &pdata->encoder, &pdata->audio, &pdata->video);
    
    if(!ret)
    {
        if(pdata->file.has_audio)
        {
            pdata->audio.mutex = PlatformCreateMutex();
            PlatformInitAudio(&pdata->file);
            pdata->audio.queue = FQInitialize(FQ_ORDERED_PTS);
            
            //PlatformResizeClientArea(&pdata->file, 0, 0);
        }
        if(pdata->file.has_video)
        {
            pdata->video.frame_duration = pdata->file.target_time;
            PlatformInitVideo(&pdata->file);
            pdata->video.queue = FQInitialize(FQ_ORDERED_PTS);
        }
        
        RETURN(SUCCESS);
    }
    else
    {
        wlog(LOG_ERROR, "Failed to open media file");
        
        RETURN(UNKNOWN_ERROR);
    }
}

internal i32
FileOpen_StartThread(program_data *pdata)
{
    AllocateBuffers(pdata);
    
    pdata->decoder.condition = PlatformCreateConditionVar();
    pdata->decoder.start_streaming = PlatformCreateConditionVar();
    
    pdata->threads.media_thread =
        PlatformCreateThread(MediaThreadStart, pdata, "media");
    wlog(LOG_INFO, "Started new thread.");
    
    pdata->file.file_ready = 1;
    
    // TODO(Val): Maybe start this playing?
    SetPlaybackPaused(pdata);
    
    RETURN(SUCCESS);
}

internal i32
FileOpen(program_data *pdata)
{
    i32 ret = 0;
    ret = FileOpen_LoadData(pdata);
    
    if(!ret)
    {
        FileOpen_StartThread(pdata);
    }
    else
    {
        pdata->file.open_failed = 1;
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

internal b32
FileClose(program_data *pdata)
{
    pdata->file.file_ready = 0;
    
    // TODO(Val): This needs to be shut down more properly
    SetPlaybackPaused(pdata);
    
    PlatformConditionSignal(&pdata->decoder.condition);
    PlatformWaitThread(pdata->threads.media_thread, NULL);
    PlatformConditionDestroy(&pdata->decoder.condition);
    
    MediaClose(&pdata->file, &pdata->decoder, &pdata->encoder, &pdata->audio, &pdata->video);
    
    DeallocateBuffers(pdata);
    
    PlatformCloseAudio(pdata);
    
    RETURN(SUCCESS);
}

internal void
ProcessMessages(program_data *pdata)
{
    StartTimer("ProcessMessages()");
    
    while(!MessagesEmpty(&pdata->messages) && pdata->running)
    {
        StartTimer("Message");
        
        message m;
        GetApplicationMessage(&pdata->messages, &m);
        switch(m.msg)
        {
            case MSG_NO_MORE_MESSAGES:
            {
                wlog(LOG_WARNING, "Empty message returned, check message queue bookkeeping");
            } break;
            case MSG_TOGGLE_FULLSCREEN:
            {
                wlog(LOG_INFO, "MSG_TOGGLE_FULLSCREEN");
                ToggleFullscreen(pdata);
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
                wlog(LOG_INFO, "MSG_PAUSE");
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
                wlog(LOG_INFO, "MSG_CLOSE");
                pdata->running = 0;
            } break;
            case MSG_VOLUME_CHANGE:
            {
                wlog(LOG_INFO, "MSG_VOLUME_CHANGE");
                
                IncreaseVolume(&pdata->audio, m.args[0].f);
            } break;
            case MSG_FILE_OPEN:
            {
                wlog(LOG_INFO, "MSG_FILE_OPEN");
                
                FileOpen(pdata);
            } break;
            case MSG_FILE_CLOSE:
            {
                FileClose(pdata);
            } break;
            case MSG_START_SERVER:
            {
                StartServer();
            } break;
            case MSG_START_CLIENT:
            {
                StartClient();
            } break;
            case MSG_CONNECT_TO_SERVER:
            {
                if(!ConnectToIP(pdata->server_address))
                {
                    wlog(LOG_INFO, "MSG_CONNECT_TO_SERVER: Connected to server successfully");
                    pdata->connected = 1;
                }
                else
                    wlog(LOG_ERROR, "MSG_CONNECT_TO_SERVER: Failed to connect to server");
            } break;
            case MSG_DISCONNECT:
            {
                wlog(LOG_INFO, "MSG_DISCONNECT");
                CloseConnection();
                pdata->connected = 0;
            } break;
            case MSG_TOGGLE_MENU:
            {
                MenuScreen *s;
                if(s = GetTopmostMenuScreen(pdata->menu))
                {
                    PopMenuScreen(pdata->menu);
                }
                else
                {
                    PushMenuScreen(pdata->menu, pdata->menu->main_menu_screen);
                }
            } break;
            default:
            {
                wlog(LOG_ERROR, "Uknown message in message queue");
                dbg_warn("Unknown message in message queue: %d\n", m.msg);
            }
        }
        
        EndTimer();
    }
    
    EndTimer();
}

internal void
ProcessAudio(program_data *pdata)
{
    StartTimer("ProcessAudio()");
    
    //PlatformLockMutex(&pdata->audio.mutex);
    
    AVFrame *frame = NULL;
    if(!FQDequeue(pdata->audio.queue, &frame, NULL))
    {
        PlatformQueueAudio(frame);
        
        increment_audio_times(&pdata->playback, (f64)frame->nb_samples/(f64)frame->sample_rate);
        //pdata->audio.requested_timestamp = get_future_playback_time(&pdata->playback);
        //PrepareAudioOutput(&pdata->audio);
        
        pdata->audio.is_ready = 0;
        
        //PlatformUnlockMutex(&pdata->audio.mutex);
        
        av_frame_unref(frame);
        EndTimer();
    }
    else
    {
        wlog(LOG_ERROR, "No frames in queue");
    }
}

internal void
ProcessVideo(program_data *pdata)
{
    StartTimer("ProcessVideo()");
    
    AVFrame *frame = NULL;
    if(!FQDequeue(pdata->video.queue, &frame, NULL))
    {
        PlatformUpdateVideoFrame(frame);
        
        increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
        
        av_frame_unref(frame);
        ClearVideoOutput(&pdata->video);
        pdata->video.is_ready = 0;
    }
    
    EndTimer();
}

not_used internal void
SkipVideoFrame(program_data *pdata)
{
    //ClearVideoOutput(&pdata->video);
    
    increment_video_times(&pdata->playback, av_q2d(pdata->decoder.video_time_base));
    
    pdata->video.is_ready = 0;
}

internal i32
ProcessPlayback(program_data *pdata)
{
    playback_data *playback = &pdata->playback;
    client_info *client = &pdata->client;
    output_audio *audio = &pdata->audio;
    output_video *video = &pdata->video;
    
    b32 need_video = 0;
    b32 need_audio = 0;
    
    // TODO(Val): There is a bug here when there are no more frames in frame queues. Getting next frame pts when there is no frame is undefined (I guess?)
    
    if(should_display(playback, FQNextTimestamp(video->queue)))
    {
        if(!FQEmpty(pdata->video.queue))
        {
            StartTimer("Processing video");
            
            wlog(LOG_TRACE, "ProcessVideo()");
            
            ProcessVideo(pdata);
            need_video = 1;
            
            EndTimer();
        }
        else
        {
            wlog(LOG_ERROR, "Video frames were not ready in time");
        }
    } 
    else
    {
        wlog(LOG_TRACE, "Not time to display next video frame");
    }
    
    if(should_queue(playback, FQNextTimestamp(pdata->audio.queue)) || !playback->started_playing)
    {
        if(!FQEmpty(pdata->audio.queue))
        {
            StartTimer("Processing audio");
            
            wlog(LOG_TRACE, "ProcessAudio()");
            
            ProcessAudio(pdata);
            need_audio = 1;
            
            EndTimer();
        }
    }
    
    if(need_audio || need_video)
    {
        PlatformConditionSignal(&pdata->decoder.condition);
    }
    RETURN(SUCCESS);
}

#define SETUP_MSG_PROCESSING(type, var, message) \
type *var = (type *)message;

internal i32
ProcessNetwork(program_data *pdata)
{
    StartTimer("ProcessNetwork()");
    
    if(unlikely(HostingFile(pdata) && !Connected(pdata)))
    {
        // NOTE(Val): Need to accept a client
        if(AcceptConnection() == CONNECTED)
        {
            pdata->connected = 1;
            //pdata->partner_address = 
        }
    }
    
    if(pdata->connected)
    {
        int ret = ReceiveControlMessages();
        if(ret == DISCONNECTED)
            goto Disconnected;
        else if(ret == UNKNOWN_ERROR)
        {
            EndTimer();
            RETURN(UNKNOWN_ERROR);
        }
        
        net_message *msg_r; 
        while((msg_r = GetNextMessage()))
        {
            StartTimer("GetNextMessage() loop");
            switch(msg_r->type)
            {
                case MESSAGE_REQUEST_INIT:
                {
                    // NOTE(Val): Server responds to the init request by sending the required information  
                    
                    SETUP_MSG_PROCESSING(struct _request_init_msg, msg, msg_r);
                    
                    destination_IP ip = {};
                    ip.v4.ip = GetPartnerIPInt();
                    
                    SendInitMessage(0.0,
                                    0.0,
                                    0);
                    
                    FileOpen_LoadData(pdata);
                } break;
                case MESSAGE_INIT:
                {
                    // NOTE(Val): Client replies by sending the server its own IP address
                    
                    SETUP_MSG_PROCESSING(struct _init_msg, msg, msg_r);
                    
                    destination_IP ip = {};
                    ip.v4.ip = GetPartnerIPInt();
                    SendFinishInitMessage(ip);
                } break;
                case MESSAGE_FINISH_INIT:
                {
                    // NOTE(Val): Server receives its own IP as visible to the client and creates an output file at that IP
                    
                    SETUP_MSG_PROCESSING(struct _finish_init_msg, msg, msg_r);
                    
                    pdata->address_storage.v4.ip = msg->ip;;
                    //Streaming_Host_Initialize(&pdata->decoder, &pdata->file, server_address);
                    
                    FileOpen_StartThread(pdata);
                    
                    SendReadyPlaybackMessage();
                    PlatformConditionSignal(&pdata->decoder.start_streaming);
                } break;
                case MESSAGE_READY_PLAYBACK:
                {
                    // NOTE(Val): Since file is ready, the client opens the file on server's address
                    
                    SETUP_MSG_PROCESSING(struct _ready_playback_msg, msg, msg_r);
                    
                    
                    Streaming_GetFileName(pdata->file.filename, pdata->server_address, Streaming_Get_Port(), NULL);
                    
                    AddMessage0(&pdata->messages, MSG_FILE_OPEN, PlatformGetTime());
                } break;
                
                // NOTE(Val): Starting here are the supplementary messages.
                
                case MESSAGE_REQUEST_PORT:
                {
                    SETUP_MSG_PROCESSING(struct _request_port_msg, msg, msg_r);
                    
                } break;
                case MESSAGE_INFO:
                {
                    SETUP_MSG_PROCESSING(struct _request_info_msg, msg, msg_r);
                    
                } break;
                case MESSAGE_PAUSE:
                {
                    SETUP_MSG_PROCESSING(struct _pause_msg, msg, msg_r);
                    
                    wlog(LOG_INFO, "Processing pause client message");
                    
                    LocalPause(pdata);
                } break;
                case MESSAGE_PLAY:
                {
                    SETUP_MSG_PROCESSING(struct _play_msg, msg, msg_r);
                    
                    LocalUnpause(pdata);
                } break;
                case MESSAGE_SEEK:
                {
                    SETUP_MSG_PROCESSING(struct _seek_msg, msg, msg_r);
                    
                    
                } break;
                case MESSAGE_DISCONNECT:
                {
                    SETUP_MSG_PROCESSING(struct _disconnect_msg, msg, msg_r);
                    
                    
                } break;
                default:
                {
                    wlog(LOG_ERROR, "Unknown client message received");
                } break;
            }
            
            custom_free(msg_r);
            
            EndTimer();
        }
        
        ret = SendControlMessages();
        if(ret == DISCONNECTED)
            goto Disconnected;
    }
    
    EndTimer();
    
    RETURN(SUCCESS);
    
    Disconnected:
    
    CloseConnection();
    SetDisconnected(pdata);
    
    wlog(LOG_INFO, "Client disconnected");
    
    RETURN(DISCONNECTED);
}

#undef SETUP_MSG_PROCESSING

internal i32
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

internal void
StartPlayback(program_data *pdata)
{
    StartTimer("Starting Playback");
    start_playback(&pdata->playback, *pdata->playback.current_frame_time + 0.3);
    
    PlaybackStart(pdata);
    
    wlog(LOG_INFO, "Playback started");
    EndTimer();
}

internal i32
MainLoopThread(void *arg)
{
    InitializeTimingSystem("main");
    
    program_data *pdata = arg;
    playback_data *playback = &pdata->playback;
    client_info *client = &pdata->client;
    client->start_time = PlatformGetTime();
    client->next_refresh_time = (client->start_time + client->refresh_target);
    
    // now start main loop
    StartTimer("Main loop");
    
    wlog(LOG_INFO, "Starting main loop");
    while(likely(pdata->running))
    {
        StartTimer("Loop");
        
        assert_memory_bounds();
        
        PlatformGetInput(pdata);
        
        ProcessInput(pdata);
        ProcessMouse(pdata);
        ProcessMessages(pdata);
        
        StartTimer("ProcessNetwork()");
        if(PlaybackNetworked(pdata))
            ProcessNetwork(pdata);
        EndTimer();
        
        if(!pdata->file.open_failed)
        {
            if(PlaybackStartFlagged(pdata))
            {
                dbg_success("Starting playback.\n");
                PlaybackStart(pdata);
                
                //if(Connected(pdata))
                //SendPlayMessage();
            }
        }
        else 
        {
            wlog(LOG_WARNING, "Failed to open file");
            //pdata->file.open_failed = 0;
            
            // TODO(Val): Opening file failed.
            
            dbg_warn("Opening file failed.\n");
        }
        
        if(PlaybackPlaying(pdata))
        {
            StartTimer("ProcessPlayback()");
            ProcessPlayback(pdata);
            EndTimer();
            //playback->playback_time += pdata->client.refresh_target;
        }
        else
        {
            //dbg_info("Not playing or paused.\n"
            //"\tplaying: %s\n"
            //"\tpaused: %s\n",
            //pdata->playing ? "true" : "false",
            //pdata->paused ? "true" : "false");
        }
        //dbg_print("Loop time: %ld\n", playback->time_end - playback->time_start);
        
        f64 time = PlatformGetTime();
        //f64 sleep_time = (client->next_refresh_time - time - 0.002);
        //dbg_info("PlatformSleep(%lf)\n"
        //"\tnext_refresh_time: %lf\n"
        //"\tcurrent time: %lf\n",
        //sleep_time,
        //client->next_refresh_time,
        //time);
        
        PlatformRender(pdata);
        
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
        PlatformFlipBuffers(pdata);
        
        Client_UpdateTime(client);
        EndTimer();
        
        EndTimer();
    }
    
    if(PlaybackNetworked(pdata))
    {
        if(pdata->connected)
        {
            SendDisconnectMessage();
            SendControlMessages();
        }
        
        if(HostingFile(pdata))
            CloseServer();
        
        CloseConnection();
        pdata->connected = 0;
    }
    
    EndTimer();
    
    FinishTiming();
    RETURN(SUCCESS);
}

internal i32
MainThread(program_data *pdata)
{
    // NOTE(Val): Initialize things here that will last the entire runtime of the application
    if(InitializeApplication(pdata) < 0)
        return -1;
    
    if(HostingFile(pdata))
    {
        AddMessage0(&pdata->messages, MSG_START_SERVER, PlatformGetTime());
        //AddMessage0(&pdata->messages, MSG_FILE_OPEN, PlatformGetTime());
    }
    else if(ReceivingFile(pdata))
    {
        AddMessage0(&pdata->messages, MSG_START_CLIENT, PlatformGetTime());
        AddMessage0(&pdata->messages, MSG_CONNECT_TO_SERVER, PlatformGetTime());
    }
    else
    {
        wlog(LOG_INFO, "Adding file open message");
        AddMessage0(&pdata->messages, MSG_FILE_OPEN, PlatformGetTime());
    }
    
    MainLoopThread(pdata);
    
    pdata->running = 0;
    return 0;
}
