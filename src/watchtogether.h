#ifndef WT
#define WT

//#include "audio_queue.h"
//#include "video_queue.h"
//#include "decoding.h"
#include <libavutil/rational.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "wt_version.h"
#include "SDL_platform.h"
#include "playback.h"
#include "message_queue.h"
#include "packet_queue.h"
#include "decoding.h"
#include "kbkeys.h"
#include "audio.h"
#include "video.h"
#include "utils.h"
#include "decoding.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

struct _platform_data;
struct _thread_info;
struct _cond_info;


typedef struct _threads_info_all {
    struct _thread_info main_thread;
    struct _thread_info decoder_thread;
    struct _thread_info blt_thread;
    struct _thread_info audio_thread;
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
    
    real64 refresh_target;
    real64 current_frame_time;
    real64 next_refresh_time;
} client_info;

typedef struct _output_audio output_audio;

#define VIDEO_RGB 1
#define VIDEO_YUV 2

typedef struct _ouput_video output_video;

typedef struct _open_file_info open_file_info;

typedef struct _avpacket_queue {
    //AVPacket *buffer;
    AVPacket **array;
    SDL_mutex *mutex;
    int32 maxn;  // max number of packets
    int32 n;     // total number of packets
    int32 next;  // the packet that will be dequeued/peeked next
    int32 end;   // the where the next packet will be enqueued
} avpacket_queue;

#define NUM_FRAMES 30

typedef struct _video_queue_data {
    uint32 vq_width;
    uint32 vq_height;
    uint32 vq_format;
    uint32 bpp;
    
    void* vq_buffer;
    uint32 vq_size;
    uint32 vq_maxframes;
    uint32 vq_nframes;
    uint32 vq_frame_size;
    uint32 vq_start;
    uint32 vq_end;
    uint32 vq_pitch;
    
    uint32 vq_Y_width;
    uint32 vq_U_width;
    uint32 vq_V_width;
    
    uint32 vq_Y_height;
    uint32 vq_U_height;
    uint32 vq_V_height;
    
    uint32 vq_Y_pitch;
    uint32 vq_U_pitch;
    uint32 vq_V_pitch;
    
    uint32 vq_Y_frame_size;
    uint32 vq_U_frame_size;
    uint32 vq_V_frame_size;
    
    uint32 vq_Y_size;
    uint32 vq_U_size;
    uint32 vq_V_size;
    
    void *vq_Y_buffer;
    void *vq_U_buffer;
    void *vq_V_buffer;
    
    uint32 vq_timestamps[NUM_FRAMES];
} video_queue_data;

typedef struct _audio_queue_data {
    void* audio_queue_buffer;
    uint32 frequency;
    uint32 channels;
    uint32 bytes_per_sample;
    uint32 audio_queue_size;
    uint32 audio_queue_used_space;
    uint32 audio_queue_start;
    uint32 audio_queue_end;
} audio_queue_data;

typedef struct _playback_data playback_data;
typedef struct _message_queue message_queue;

typedef struct _program_data {
    input_struct input;
    client_info client;
    output_audio audio;
    output_video video;
    open_file_info file;
    threads_info_all threads;
    audio_queue_data aq_data;
    video_queue_data vq_data;
    decoder_info decoder;
    playback_data playback;
    hardware_info hardware;
    message_queue messages;
    
    // TODO(Val): Will we need multiple packet queues?
    avpacket_queue *pq_main;
    avpacket_queue *pq_playback;
    avpacket_queue *pq_stream;
    avpacket_queue *pq_video;
    avpacket_queue *pq_audio;
    
    // TODO(Val): remove this or make it more organized. 
    real32 volume;
    uint32 audio_format;
    uint32 tick;
    
    bool32 is_fullscreen;
    
    volatile bool32 running;
    volatile bool32 playing;
    volatile bool32 paused;
    volatile bool32 start_playback;
    volatile bool32 playback_finished;
} program_data;

int32 
MainThread(program_data *);

int32
MainLoop(program_data  *);

#include "platform.h"

#endif