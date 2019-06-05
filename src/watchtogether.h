#ifndef WT
#define WT

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#ifdef DEBUG

#ifdef _WIN32
#define dbg_print(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#define dbg_error(x) do { fprintf(stderr, "%s %d : %s", __FILE__, __LINE__, x); } while(0)
#define dbg_info(x) fprintf(stderr, x)
#define dbg_success(x) fprintf(stderr, x)
#define dbg(x) x
#endif

#ifdef __linux__
#define dbg_print(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#define dbg_error(x) do { fprintf(stderr, KRED "%s %d : %s" KNRM, __FILE__, __LINE__, x); } while(0)
#define dbg_info(x) fprintf(stderr, KYEL x KNRM)
#define dbg_success(x) fprintf(stderr, KGRN x KNRM)
#define dbg(x) x
#endif

#else

#define dbg_print(...)
#define dbg_error(x)
#define dbg_info(x)
#define dbg_success(x)
#define dbg(x)

#endif


struct _platform_data;
struct _thread_info;

typedef struct _threads_info_all {
    struct _thread_info main_thread;
    struct _thread_info decoder_thread;
    struct _thread_info blt_thread;
    struct _thread_info audio_thread;
} threads_info_all;

typedef struct _decoder_info decoder_info;

typedef struct _pixel_buffer {
    void* buffer;
    uint32 width;
    uint32 height;
    uint32 pitch;
} pixel_buffer;

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

typedef struct _client_info {
    uint32 output_width;
    uint32 output_height;
    uint32 bytes_per_pixel;
    bool32 fullscreen;
    
} client_info;

typedef struct _output_audio {
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
} output_audio;

typedef struct _ouput_video {
    void *video_frame;
    uint32 width;
    uint32 height;
    uint32 pitch;
} output_video;


#define VIDEO_RGB 1
#define VIDEO_YUV 2
typedef struct _open_file_info {
    char *filename;
    
    uint32 width;
    uint32 height;
    uint32 pitch;
    uint32 video_format;
    real32 fps;
    real32 target_time;
    
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
    
    bool32 has_audio;
    bool32 has_video;
    
    bool32 file_opened;
    bool32 file_ready;
    // TODO(Val): audio format
} open_file_info;

typedef struct _video_queue_data video_queue_data;

typedef struct _audio_queue_data audio_queue_data;

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
    
    // TODO(Val): Will we need multiple packet queues?
    packet_queue *pq_main;
    packet_queue *pq_playback;
    packet_queue *pq_stream;
    
    real64 prevFrameTime;
    real64 nextFrameTime;
    
    // TODO(Val): remove this or make it more organized. 
    real32 volume;
    uint32 audio_format;
    uint32 tick;
    
    volatile bool32 running;
    bool32 playing;
    bool32 paused;
} program_data;

static int32 MainLoop(program_data  *);

#endif