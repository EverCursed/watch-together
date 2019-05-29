#include "video_queue.h"

// TODO(Val): Add thread synchronization
// ...Actually is this even needed? When pushing is done
// the bookkeeping isn't updated until copying is finished. 
// As soon as bookkeeping is updated, it is safe to copy that frame.

// REVIEW(Val): It is possible to have some false sharing performance issues

// TODO(Val): There is a bug where frames start spazzing out after playing for some time

#define RGB 0
#define YUV 1

static void
init_video_queue(video_queue_data *data,
                 uint32 w,
                 uint32 h,
                 uint32 b)
{
    data->vq_width = w;
    data->vq_height = h;
    data->bpp = b;
    data->vq_pitch = round_up_align(w*data->bpp);
    
    data->vq_frame_size = data->vq_pitch*h;
    data->vq_size = data->vq_frame_size*NUM_FRAMES;
    data->vq_buffer = malloc(data->vq_size);
    data->vq_maxframes = NUM_FRAMES;
    
    data->vq_start = 0;
    data->vq_end = 0;
    data->vq_nframes = 0;
    
    data->vq_format = RGB;
}

static void
init_video_queue_YUV(video_queue_data *data,
                     uint32 Ywidth,
                     uint32 Yheight)
{
    uint32 Uwidth = (Ywidth+1)/2;
    uint32 Uheight = (Yheight+1)/2;
    uint32 Vwidth = (Ywidth+1)/2;
    uint32 Vheight = (Yheight+1)/2;
    
    uint32 Ypitch = round_up_align(Ywidth);
    uint32 Upitch = round_up_align(Uwidth);
    uint32 Vpitch = round_up_align(Vwidth);
    
    data->vq_Y_width = Ywidth;
    data->vq_Y_height = Yheight;
    data->vq_U_width = Uwidth;
    data->vq_U_height = Uheight;
    data->vq_V_width = Vwidth;
    data->vq_V_height = Vheight;
    
    data->vq_Y_pitch = Ypitch;
    data->vq_U_pitch = Upitch;
    data->vq_V_pitch = Vpitch;
    
    dbg_print("Y pitch: %d\nU pitch: %d\nV pitch: %d\n",
              data->vq_Y_pitch,
              data->vq_U_pitch,
              data->vq_V_pitch);
    
    data->vq_Y_frame_size = Ypitch*Yheight;
    data->vq_U_frame_size = Upitch*Uheight;
    data->vq_V_frame_size = Vpitch*Vheight;
    data->vq_Y_size = data->vq_Y_frame_size*NUM_FRAMES;
    data->vq_U_size = data->vq_U_frame_size*NUM_FRAMES;
    data->vq_V_size = data->vq_V_frame_size*NUM_FRAMES;
    data->vq_Y_buffer = malloc(data->vq_Y_size);
    data->vq_U_buffer = malloc(data->vq_U_size);
    data->vq_V_buffer = malloc(data->vq_V_size);
    
    data->vq_maxframes = NUM_FRAMES;
    
    data->vq_start = 0;
    data->vq_end = 0;
    data->vq_nframes = 0;
    
    data->vq_format = YUV;
}

static inline int32
find_timestamp(video_queue_data *data,
               uint32 timestamp)
{
    for(int i = 0; i < NUM_FRAMES; i++)
    {
        if(data->vq_timestamps[i] == timestamp)
            return i;
    }
    
    return -1;
}

static int32
enqueue_frame(video_queue_data *data,
              void *src,
              uint32 pitch,
              uint64 timestamp)
{
    if(data->vq_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == data->vq_maxframes)
    {
        dbg_print("Warning: Not enough space in video queue.\n");
        return -1;
    }
    
    for(int i = 0; i < data->vq_height; i++)
    {
        memcpy(data->vq_buffer + data->vq_end*data->vq_frame_size + i*data->vq_pitch,
               src + i*pitch,
               data->vq_width*data->bpp);
    }
    
    data->vq_end = (data->vq_end+1) % data->vq_maxframes;
    data->vq_nframes++;
    data->vq_timestamps[data->vq_end] = timestamp;
    
    return 0;
}

static int32
get_next_frame(video_queue_data *data,
               void **buffer,
               uint32 *pitch)
{
    if(data->vq_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_print("Warning: Video queue is empty.\n");
        return -1;
    }
    
    *buffer = data->vq_buffer + data->vq_frame_size*data->vq_start;
    *pitch = data->vq_pitch;
    
    return 0;
}

static int32
get_frame(video_queue_data *data,
          void **buffer,
          uint32 *pitch,
          int32 timestamp)
{
    if(data->vq_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_print("Warning: Video queue is empty.\n");
        return -1;
    }
    
    *buffer = data->vq_buffer + data->vq_frame_size*data->vq_start;
    *pitch = data->vq_pitch;
    
    return 0;
}

static int32
discard_next_frame(video_queue_data *data)
{
    if(data->vq_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    data->vq_start = (data->vq_start+1) % data->vq_maxframes;
    data->vq_nframes--;
    
    return 0;
}

static int32
dequeue_frame(video_queue_data *data,
              void *dst)
{
    if(data->vq_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    memcpy(dst,
           data->vq_buffer + data->vq_start*data->vq_frame_size,
           data->vq_frame_size);
    
    data->vq_start = (data->vq_start+1) % data->vq_maxframes;
    data->vq_nframes--;
    
    return 0;
}

static int32
enqueue_frame_YUV(video_queue_data *data,
                  void *srcY,
                  uint32 pitchY,
                  void *srcU,
                  uint32 pitchU,
                  void *srcV,
                  uint32 pitchV,
                  uint64 timestamp)
{
    if(data->vq_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == data->vq_maxframes)
    {
        dbg_print("Warning: Not enough space in video queue.\n");
        return -1;
    }
    
    uint32 Yp = data->vq_Y_pitch;
    uint32 Up = data->vq_U_pitch;
    uint32 Vp = data->vq_V_pitch;
    
    uint32 Yh = data->vq_Y_height;
    uint32 Uh = data->vq_U_height;
    uint32 Vh = data->vq_V_height;
    
    uint32 Yw = Yp < pitchY ? Yp : pitchY;
    uint32 Uw = Up < pitchU ? Up : pitchU;
    uint32 Vw = Vp < pitchV ? Vp : pitchV;
    
    void *Ybuf = data->vq_Y_buffer;
    void *Ubuf = data->vq_U_buffer;
    void *Vbuf = data->vq_V_buffer;
    
    uint32 nframes = data->vq_nframes;
    uint32 Ysize = data->vq_Y_frame_size;
    uint32 Usize = data->vq_U_frame_size;
    uint32 Vsize = data->vq_V_frame_size;
    
    for(int i = 0; i < Yh; i++)
    {
        memcpy(Ybuf + data->vq_end*Ysize + i*Yp, srcY + i*pitchY, Yw);
    }
    
    for(int i = 0; i < Uh; i++)
    {
        memcpy(Ubuf + data->vq_end*Usize + i*Up, srcU + i*pitchU, Uw);
    }
    
    for(int i = 0; i < Vh; i++)
    {
        memcpy(Vbuf + data->vq_end*Vsize + i*Vp, srcV + i*pitchV, Vw);
    }
    /*
    memcpy(data->vq_Y_buffer + data->vq_Y_frame_size*data->vq_end,
           srcY,
           data->vq_Y_frame_size);
    memcpy(data->vq_U_buffer + data->vq_U_frame_size*data->vq_end,
           srcU,
           data->vq_U_frame_size);
    memcpy(data->vq_V_buffer + data->vq_V_frame_size*data->vq_end,
           srcV,
           data->vq_V_frame_size);
    */
    data->vq_end = (data->vq_end+1) % data->vq_maxframes;
    data->vq_nframes++;
    
    return 0;
}

static int32
get_next_frame_YUV(video_queue_data *data,
                   void **dstY,
                   uint32 *pitchY,
                   void **dstU,
                   uint32 *pitchU,
                   void **dstV,
                   uint32 *pitchV)
{
    if(data->vq_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    *dstY = data->vq_Y_buffer + data->vq_Y_frame_size*data->vq_start;
    *dstU = data->vq_U_buffer + data->vq_U_frame_size*data->vq_start;
    *dstV = data->vq_V_buffer + data->vq_V_frame_size*data->vq_start;
    
    *pitchY = data->vq_Y_pitch;
    *pitchU = data->vq_U_pitch;
    *pitchV = data->vq_V_pitch;
    
    return 0;
}

static int32
discard_next_frame_YUV(video_queue_data *data)
{
    if(data->vq_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    data->vq_start = (data->vq_start+1) % data->vq_maxframes;
    data->vq_nframes--;
    
    return 0;
}


/// DO NOT USE YET
static int32
dequeue_frame_YUV(video_queue_data *data,
                  void *dstY,
                  void *dstU,
                  void *dstV)
{
    if(data->vq_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->vq_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    memcpy(dstY,
           data->vq_Y_buffer + data->vq_nframes*data->vq_Y_frame_size,
           data->vq_Y_frame_size);
    memcpy(dstU,
           data->vq_U_buffer + data->vq_nframes*data->vq_U_frame_size,
           data->vq_U_frame_size);
    memcpy(dstV,
           data->vq_V_buffer + data->vq_nframes*data->vq_V_frame_size,
           data->vq_V_frame_size);
    
    return 0;
}

static void
close_video_queue(video_queue_data *data)
{
    data->vq_maxframes = 0;
    if(data->vq_buffer)
    {
        free(data->vq_buffer);
        data->vq_buffer = NULL;
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
