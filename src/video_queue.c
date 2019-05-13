#include "video_queue.h"

// TODO(Val): Add thread synchronization
// ...Actually is this even needed? When pushing is done
// the bookkeeping isn't updated until copying is finished. 
// As soon as bookkeeping is updated, it is safe to copy that frame.

// REVIEW(Val): It is possible to have some false sharing performance issues

#define NUM_FRAMES 30
#define RGB 0
#define YUV 1

static void
init_video_queue(video_queue_data *data,
                 uint32 w,
                 uint32 h,
                 uint32 b)
{
    data->video_queue_width = w;
    data->video_queue_height = h;
    data->bpp = b;
    data->video_queue_pitch = round_up_align(w*data->bpp);
    
    data->video_queue_frame_size = data->video_queue_pitch*h;
    data->video_queue_size = data->video_queue_frame_size*NUM_FRAMES;
    data->video_queue_buffer = malloc(data->video_queue_size);
    data->video_queue_maxframes = NUM_FRAMES;
    
    data->video_queue_start = 0;
    data->video_queue_end = 0;
    data->video_queue_nframes = 0;
    
    data->video_queue_format = RGB;
}

static void
init_video_queue_YUV(video_queue_data *data,
                     uint32 Ywidth,
                     uint32 Yheight)
//uint32 Ypitch,
//uint32 Upitch,
//uint32 Vpitch)
{
    uint32 Uwidth = (Ywidth+1)/2;
    uint32 Uheight = (Yheight+1)/2;
    uint32 Vwidth = (Ywidth+1)/2;
    uint32 Vheight = (Yheight+1)/2;
    
    uint32 Ypitch = round_up_align(Ywidth);
    uint32 Upitch = round_up_align(Uwidth);
    uint32 Vpitch = round_up_align(Vwidth);
    
    // TODO(Val): make this for YUV
    data->video_queue_Y_width = Ywidth;
    data->video_queue_Y_height = Yheight;
    data->video_queue_U_width = Uwidth;
    data->video_queue_U_height = Uheight;
    data->video_queue_V_width = Vwidth;
    data->video_queue_V_height = Vheight;
    //data->bpp = b;
    data->video_queue_Y_pitch = Ypitch;
    data->video_queue_U_pitch = Upitch;
    data->video_queue_V_pitch = Vpitch;
    
    dbg_print("Y pitch: %d\nU pitch: %d\nV pitch: %d\n",
              data->video_queue_Y_pitch,
              data->video_queue_U_pitch,
              data->video_queue_V_pitch);
    
    data->video_queue_Y_frame_size = Ypitch*Yheight;
    data->video_queue_U_frame_size = Upitch*Uheight;
    data->video_queue_V_frame_size = Vpitch*Vheight;
    data->video_queue_Y_size = data->video_queue_Y_frame_size*NUM_FRAMES;
    data->video_queue_U_size = data->video_queue_U_frame_size*NUM_FRAMES;
    data->video_queue_V_size = data->video_queue_V_frame_size*NUM_FRAMES;
    data->video_queue_Y_buffer = malloc(data->video_queue_Y_size);
    data->video_queue_U_buffer = malloc(data->video_queue_U_size);
    data->video_queue_V_buffer = malloc(data->video_queue_V_size);
    
    data->video_queue_maxframes = NUM_FRAMES;
    
    data->video_queue_start = 0;
    data->video_queue_end = 0;
    data->video_queue_nframes = 0;
    
    data->video_queue_format = YUV;
}

// NOTE(Val): DO NOT USE THIS ONE, IT IS SUPER DUPER BROKEN
static int32
enqueue_frame(video_queue_data *data,
              void *src,
              uint32 pitch)
{
    if(data->video_queue_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
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
    
    for(int i = 0; i < data->video_queue_height; i++)
    {
        memcpy(data->video_queue_buffer + data->video_queue_end*data->video_queue_frame_size + i*data->video_queue_pitch,
               src + i*pitch,
               data->video_queue_width*data->bpp);
    }
    
    data->video_queue_end = (data->video_queue_end+1) % data->video_queue_maxframes;
    data->video_queue_nframes++;
    
    return 0;
}

static int32
get_next_frame(video_queue_data *data,
               void **buffer,
               uint32 *pitch)
{
    if(data->video_queue_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_print("Warning: Video queue is empty.\n");
        return -1;
    }
    
    *buffer = data->video_queue_buffer + data->video_queue_frame_size*data->video_queue_start;
    *pitch = data->video_queue_pitch;
    
    return 0;
}

static int32
discard_next_frame(video_queue_data *data)
{
    if(data->video_queue_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    data->video_queue_start = (data->video_queue_start+1) % data->video_queue_maxframes;
    data->video_queue_nframes--;
    
    return 0;
}

static int32
dequeue_frame(video_queue_data *data,
              void *dst)
{
    if(data->video_queue_format != RGB)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    memcpy(dst,
           data->video_queue_buffer + data->video_queue_start*data->video_queue_frame_size,
           data->video_queue_frame_size);
    
    data->video_queue_start = (data->video_queue_start+1) % data->video_queue_maxframes;
    data->video_queue_nframes--;
    
    return 0;
}

static int32
enqueue_frame_YUV(video_queue_data *data,
                  void *srcY,
                  uint32 pitchY,
                  void *srcU,
                  uint32 pitchU,
                  void *srcV,
                  uint32 pitchV)
{
    if(data->video_queue_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
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
    
    uint32 Yp = data->video_queue_Y_pitch;
    uint32 Up = data->video_queue_U_pitch;
    uint32 Vp = data->video_queue_V_pitch;
    
    uint32 Yh = data->video_queue_Y_height;
    uint32 Uh = data->video_queue_U_height;
    uint32 Vh = data->video_queue_V_height;
    
    uint32 Yw = Yp < pitchY ? Yp : pitchY;
    uint32 Uw = Up < pitchU ? Up : pitchU;
    uint32 Vw = Vp < pitchV ? Vp : pitchV;
    
    void *Ybuf = data->video_queue_Y_buffer;
    void *Ubuf = data->video_queue_U_buffer;
    void *Vbuf = data->video_queue_V_buffer;
    
    uint32 nframes = data->video_queue_nframes;
    uint32 Ysize = data->video_queue_Y_frame_size;
    uint32 Usize = data->video_queue_U_frame_size;
    uint32 Vsize = data->video_queue_V_frame_size;
    
    for(int i = 0; i < Yh; i++)
    {
        memcpy(Ybuf + data->video_queue_end*Ysize + i*Yp, srcY + i*pitchY, Yw);
    }
    
    for(int i = 0; i < Uh; i++)
    {
        memcpy(Ubuf + data->video_queue_end*Usize + i*Up, srcU + i*pitchU, Uw);
    }
    
    for(int i = 0; i < Vh; i++)
    {
        memcpy(Vbuf + data->video_queue_end*Vsize + i*Vp, srcV + i*pitchV, Vw);
    }
    /*
    memcpy(data->video_queue_Y_buffer + data->video_queue_Y_frame_size*data->video_queue_end,
           srcY,
           data->video_queue_Y_frame_size);
    memcpy(data->video_queue_U_buffer + data->video_queue_U_frame_size*data->video_queue_end,
           srcU,
           data->video_queue_U_frame_size);
    memcpy(data->video_queue_V_buffer + data->video_queue_V_frame_size*data->video_queue_end,
           srcV,
           data->video_queue_V_frame_size);
    */
    data->video_queue_end = (data->video_queue_end+1) % data->video_queue_maxframes;
    data->video_queue_nframes++;
    
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
    if(data->video_queue_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    *dstY = data->video_queue_Y_buffer + data->video_queue_Y_frame_size*data->video_queue_start;
    *dstU = data->video_queue_U_buffer + data->video_queue_U_frame_size*data->video_queue_start;
    *dstV = data->video_queue_V_buffer + data->video_queue_V_frame_size*data->video_queue_start;
    
    *pitchY = data->video_queue_Y_pitch;
    *pitchU = data->video_queue_U_pitch;
    *pitchV = data->video_queue_V_pitch;
    
    return 0;
}

static int32
discard_next_frame_YUV(video_queue_data *data)
{
    if(data->video_queue_format != YUV)
    {
        dbg_error("Wrong video format function.\n");
        return -1;
    }
    
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    data->video_queue_start = (data->video_queue_start+1) % data->video_queue_maxframes;
    data->video_queue_nframes--;
    
    return 0;
}


/// DO NOT USE YET
static int32
dequeue_frame_YUV(video_queue_data *data,
                  void *dstY,
                  void *dstU,
                  void *dstV)
{
    if(data->video_queue_maxframes == 0)
    {
        dbg_error("Video queue must be initialized.\n");
        return -1;
    }
    
    if(data->video_queue_nframes == 0)
    {
        dbg_error("Video queue is empty.\n");
        return -1;
    }
    
    memcpy(dstY,
           data->video_queue_Y_buffer + data->video_queue_nframes*data->video_queue_Y_frame_size,
           data->video_queue_Y_frame_size);
    memcpy(dstU,
           data->video_queue_U_buffer + data->video_queue_nframes*data->video_queue_U_frame_size,
           data->video_queue_U_frame_size);
    memcpy(dstV,
           data->video_queue_V_buffer + data->video_queue_nframes*data->video_queue_V_frame_size,
           data->video_queue_V_frame_size);
    
    return 0;
}

static void
close_video_queue(video_queue_data *data)
{
    data->video_queue_maxframes = 0;
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
