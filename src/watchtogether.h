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

// must be above video, audio, and subtitles
#include "avframe_pts_ordered_queue.h"
#include "video.h"
#include "audio.h"

#include "utils.h"
#include "media_processing.h"
#include "network.h"
#include "file_data.h"
#include "attributes.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

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
            uint8 a;
            uint8 b;
            uint8 c;
            uint8 d;
        };
        uint32 ip;
    }; 
    
    uint16 port;
} socket_info;

typedef struct _decoder_info decoder_info;
typedef struct _encoder_info encoder_info;

typedef struct _key_event {
    uint32 key;
    bool32 pressed;
    bool32 shift;
    bool32 alt;
    bool32 ctrl;
} key_event;

#define MAX_KEYS 8
typedef struct _keyboard_info {
    key_event events[MAX_KEYS];
    uint32 n;
} keyboard_info;

typedef struct _mouse_info {
    int32 old_x;
    int32 old_y;
    int32 x;
    int32 y;
    bool32 left_button_was_pressed;
    bool32 right_button_was_pressed;
    bool32 left_button_is_pressed;
    bool32 right_button_is_pressed;
    bool32 middle_button_was_pressed;
    bool32 middle_button_is_pressed;
} mouse_info;

#define SAMPLE_U8      1
#define SAMPLE_S16     2
#define SAMPLE_S32     3
#define SAMPLE_S64     4
#define SAMPLE_FLOAT   5
#define SAMPLE_DOUBLE  6

typedef struct _sound_sample {
    void *Data;
    uint32 Size;
    uint32 SampleCount;
    uint32 Frequency;
    uint16 Channels;
    uint16 BitsPerSample;
} sound_sample;

typedef struct _input_struct {
    keyboard_info keyboard;
    mouse_info mouse;
} input_struct;

typedef struct _hardware_info {
    int32 monitor_refresh_rate;
    int32 monitor_width;
    int32 monitor_height;
    
} hardware_info;

typedef struct _client_info {
    uint32 output_width;
    uint32 output_height;
    uint32 bytes_per_pixel;
    bool32 fullscreen;
    
    real64 start_time;
    real64 refresh_target;
    real64 current_frame_time;
    real64 next_refresh_time;
} client_info;

static inline void
Client_UpdateTime(client_info *client)
{
    client->current_frame_time = client->next_refresh_time;
    client->next_refresh_time += client->refresh_target;
}

static inline void
Client_SetRefreshTime(client_info *client, real64 target_time)
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
    
    //avpacket_queue *pq_main;
    //avpacket_queue *pq_playback;
    //avpacket_queue *pq_stream;
    //avpacket_queue *pq_video;
    //avpacket_queue *pq_audio;
    
    // TODO(Val): remove this or make it more organized. 
    real32 volume;
    uint32 audio_format;
    uint32 tick;
    
    bool32 is_fullscreen;
    bool32 connected;
    
    bool32 running;
    bool32 playing;
    bool32 paused;
    bool32 start_playback;
    bool32 playback_finished;
    
    bool32 is_host;
    bool32 is_partner;
    
    char* server_address;
    destination_IP address_storage;
} program_data;

int32 
MainThread(program_data *);

int32
MainLoop(program_data  *);

not_used static bool32 WaitingForPlaybackStart(program_data *pdata)
{
    return (pdata->is_host || pdata->is_partner) ? !pdata->connected : 0;
}

not_used static bool32 Connected(program_data *pdata)
{
    return pdata->connected;
}

#endif