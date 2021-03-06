/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Header for the entire application state. 
*/

#ifndef WT
#define WT

#include <libavutil/rational.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "wt_version.h"
#include "platform.h"
#include "playback.h"
#include "message_queue.h"
#include "packet_queue.h"
#include "decoding.h"
#include "encoding.h"
#include "kbkeys.h"
#include "avframe_pts_ordered_queue.h" // must be above video, audio, and subtitles 
#include "video.h"
#include "audio.h"
#include "ui.h"

#include "utils.h"
#include "media_processing.h"
#include "network.h"
#include "file_data.h"
#include "attributes.h"

// -----------------------------------------------

struct _platform_data;
struct _thread_info;
struct _cond_info;

typedef struct _threads_info_all {
    struct _thread_info media_thread;
    //struct _thread_info main_thread;
    //struct _thread_info decoder_thread;
    //struct _thread_info blt_thread;
    //struct _thread_info audio_thread;
    //struct _thread_info input_thread;
} threads_info_all;

typedef struct _socket_info {
    union {
        struct {
            u8 a;
            u8 b;
            u8 c;
            u8 d;
        };
        u32 ip;
    }; 
    
    u16 port;
} socket_info;

typedef struct _decoder_info decoder_info;
typedef struct _encoder_info encoder_info;

typedef struct _key_event {
    u32 key;
    b32 pressed;
    b32 shift;
    b32 alt;
    b32 ctrl;
} key_event;

#define MAX_KEYS 8
typedef struct _keyboard_info {
    key_event events[MAX_KEYS];
    u32 n;
} keyboard_info;

typedef struct _mouse_info {
    i32 old_x;
    i32 old_y;
    i32 x;
    i32 y;
    b32 left_button_was_pressed;
    b32 right_button_was_pressed;
    b32 left_button_is_pressed;
    b32 right_button_is_pressed;
    b32 middle_button_was_pressed;
    b32 middle_button_is_pressed;
} mouse_info;

enum {
    SAMPLE_U8,
    SAMPLE_S16,
    SAMPLE_S32,
    SAMPLE_S64,
    SAMPLE_FLOAT,
    SAMPLE_DOUBLE,
};

typedef struct _sound_sample {
    void *Data;
    u32 Size;
    u32 SampleCount;
    u32 Frequency;
    u16 Channels;
    u16 BitsPerSample;
} sound_sample;

typedef struct _input_struct {
    keyboard_info keyboard;
    mouse_info mouse;
} input_struct;

typedef struct _hardware_info {
    i32 monitor_refresh_rate;
    i32 monitor_width;
    i32 monitor_height;
    
} hardware_info;

typedef struct _client_info {
    i32 output_width;
    i32 output_height;
    i32 bytes_per_pixel;
    b32 fullscreen;
    
    f64 start_time;
    f64 refresh_target;
    f64 current_frame_time;
    f64 next_refresh_time;
} client_info;

internal inline void
Client_UpdateTime(client_info *client)
{
    client->current_frame_time = client->next_refresh_time;
    client->next_refresh_time += client->refresh_target;
}

internal inline void
Client_SetRefreshTime(client_info *client, f64 target_time)
{
    client->refresh_target = target_time;
    
    dbg_info("Client refresh target time set to %lfs.\n", client->refresh_target);
}


#define VIDEO_RGB 1
#define VIDEO_YUV 2

typedef struct _output_audio output_audio;
typedef struct _output_video output_video;

typedef struct _open_file_info open_file_info;

typedef struct _avpacket_queue avpacket_queue;

#define NUM_FRAMES 30

typedef struct _playback_data playback_data;
typedef struct _message_queue message_queue;

typedef struct _program_data2 {
    input_struct input;
    client_info client;
    output_audio audio;
    output_video video;
    open_file_info file;
    threads_info_all threads;
    decoder_info decoder;
    encoder_info encoder;
    playback_data playback;
    hardware_info hardware;
    message_queue messages;
    
    // TODO(Val): remove this or make it more organized. 
    b32 is_fullscreen;
    
    b32 running;
    
    b32 is_host;
    b32 is_partner;
    b32 connected;
    
    b32 start_playback_flag;
    b32 started_playback;
    b32 finished_playback;
    
    b32 paused;
} program_data2;


typedef struct _program_data {
    input_struct input;
    client_info client;
    output_audio audio;
    output_video video;
    open_file_info file;
    threads_info_all threads;
    decoder_info decoder;
    encoder_info encoder;
    playback_data playback;
    hardware_info hardware;
    message_queue messages;
    Menu *menu;
    
    b32 running;
    b32 full_screen;
    
    b32 host;
    b32 partner;
    b32 connected;
    
    b32 playback_started;
    b32 playback_finished;
    b32 playing;
    
    i32 flags;
    
    // TODO(Val): Reorganize these
    char* server_address;
    destination_IP address_storage;
} program_data;

internal i32 MainThread(program_data *);
internal i32 MainLoop(program_data  *);


#define START_PLAYBACK_FLAG 0x1

internal inline b32 ReceivingFile(program_data *pdata);
internal inline b32 HostingFile(program_data *pdata);

internal inline b32
PlaybackPlaying(program_data *pdata)
{
    return (pdata->playback_started && pdata->playing);
}

internal inline b32
PlaybackStarted(program_data *pdata)
{
    return pdata->playback_started;
}

internal inline void
SetPlaybackStartFlag(program_data *pdata, b32 on)
{
    pdata->flags = 
        on ? pdata->flags | START_PLAYBACK_FLAG : pdata->flags & ~START_PLAYBACK_FLAG;
}

internal inline void
PlaybackStart(program_data *pdata)
{
    SetPlaybackStartFlag(pdata, 0);
    
    pdata->playback_started = 1;
}

internal inline b32
PlaybackStartFlagged(program_data *pdata)
{
    return !!(pdata->flags & START_PLAYBACK_FLAG);
}

internal inline void
SetPlaybackStarted(program_data *pdata)
{
    if(!PlaybackStarted(pdata))
        PlaybackStart(pdata);
}

internal inline void
SetPlaybackUnpaused(program_data *pdata)
{
    if(pdata->playback_started)
        pdata->playing = 1;
}

internal inline b32
PlaybackPaused(program_data *pdata)
{
    return (pdata->playback_started && !pdata->playing);
}

internal inline void
SetPlaybackPaused(program_data *pdata)
{
    if(pdata->playback_started)
        pdata->playing = 0;
}

internal inline b32
PlaybackFinished(program_data *pdata)
{
    return pdata->playback_finished;
}

internal inline b32
Connected(program_data *pdata)
{
    return pdata->connected;
}

internal inline void
SetDisconnected(program_data *pdata)
{
    pdata->connected = pdata->partner = pdata->host = 0;
}

internal inline b32
PlaybackNetworked(program_data *pdata)
{
    return pdata->host || pdata->partner;
}

internal inline b32
HostingFile(program_data *pdata)
{
    return pdata->host;
}

internal inline void
SetHosting(program_data *pdata)
{
    if(!ReceivingFile(pdata))
        pdata->host = 1;
}

internal inline b32
ReceivingFile(program_data *pdata)
{
    return pdata->partner;
}

internal inline void
SetReceiving(program_data *pdata)
{
    if(!HostingFile(pdata))
        pdata->partner = 1;
}

internal b32
PlaybackWaitingConnection(program_data *pdata)
{
    return PlaybackNetworked(pdata) ? !pdata->connected : 0;
}

internal b32
Fullscreen(program_data *pdata)
{
    return pdata->full_screen;
}

internal b32
ToggleFullscreen(program_data *pdata)
{
    pdata->full_screen = !pdata->full_screen;
    
    PlatformChangeFullscreenState(!Fullscreen(pdata));
    
    return pdata->full_screen;
}

#endif