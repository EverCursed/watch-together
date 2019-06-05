#ifndef WT_VIDEO_QUEUE
#define WT_VIDEO_QUEUE

#define VQ_RGB 0
#define VQ_YUV 1

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


#endif