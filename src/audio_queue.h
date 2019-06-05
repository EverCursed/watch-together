#ifndef WT_AUDIO_QUEUE
#define WT_AUDIO_QUEUE

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


#endif