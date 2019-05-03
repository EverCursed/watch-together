#include "video_queue.h"

// TODO(Val): Add thread synchronization
// ...Actually is this even needed? When pushing is done
// the bookkeeping isn't updated until copying is finished. 
// As soon as bookkeeping is updated, it is safe to copy that frame.

// REVIEW(Val): It is possible to have some false sharing performance issues

#define NUM_FRAMES 30

static void
init_video_queue(video_queue_data *data,
                 uint32 w,
                 uint32 h,
                 uint32 b)
{
    data->video_queue_width = w;
    data->video_queue_height = h;
    data->bpp = b;
    data->video_queue_pitch = w*data->bpp + (16 - (w*data->bpp % 16));
    
    data->video_queue_frame_size = data->video_queue_pitch*h;
    data->video_queue_size = data->video_queue_frame_size*NUM_FRAMES;
    data->video_queue_buffer = malloc(data->video_queue_size);
    data->video_queue_maxframes = NUM_FRAMES;
    
    data->video_queue_start = 0;
    data->video_queue_end = 0;
    data->video_queue_nframes = 0;
}

static int32
enqueue_frame(video_queue_data *data,
              void *src)
{
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == data->video_queue_maxframes)
    {
        dbg_print("Warning: Not enough space in video queue.\n");
        return -1;
    }
    
    memcpy(data->video_queue_buffer + data->video_queue_end*data->video_queue_frame_size,
           src,
           data->video_queue_frame_size);
    
    data->video_queue_end = (data->video_queue_end+1) % data->video_queue_maxframes;
    data->video_queue_nframes++;
    
    return 0;
}

static int32
dequeue_frame(video_queue_data *data,
              void *dst)
{
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    memcpy(dst,
           data->video_queue_buffer + data->video_queue_start*data->video_queue_frame_size,
           data->video_queue_frame_size);
    
    data->video_queue_start = (data->video_queue_start+1) % data->video_queue_maxframes;
    data->video_queue_nframes--;
    
    return 0;
}

static void
close_video_queue(video_queue_data *data)
{
    if(data->video_queue_buffer)
    {
        free(data->video_queue_buffer);
        data->video_queue_buffer = NULL;
    }
}

static void
reset_video_queue(video_queue_data *data,
                  uint32 w,
                  uint32 h,
                  uint32 b)
{
    close_video_queue(data);
    init_video_queue(data, w, h, b);
}
