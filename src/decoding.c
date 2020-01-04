/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

The main decoding module. This is where packets are processed.
*/

#include <libswscale/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

//#include "defines.h"
#include "watchtogether.h"
#include "packet_queue.h"
#include "utils/timing.h"
//#include "platform.h"
//#include "packet_queue.h"
//#include "decoding.h"


#define FRAME_AUDIO 1
//#define FRAME_VIDEO_RGB 2
//#define FRAME_VIDEO_YUV 3
#define FRAME_VIDEO 4
#define FRAME_UNKNOWN -1


// TODO(Val): Make this more neat 
/*
int32
get_frame(avpacket_queue *queue, AVFrame **frame, AVCodecContext *dec_ctx)
{

}
*/
#define AUDIO_FRAME 1
#define VIDEO_FRAME 2

internal inline void
copy_pixel_buffers(uint8 *dst,
                   int32 pitch_dst,
                   uint8 *src,
                   int32 pitch_src,
                   int32 height)
{
    int32 width = pitch_src < pitch_dst ? pitch_src : pitch_dst;
    for(int i = 0; i < height; i++)
    {
        memcpy(dst + i*pitch_dst, src + i*pitch_src, width);
    }
}

#define copy_bytes(dst, src, bytes)\
do {\
    for(int i = 0; i < bytes; i++)\
    {\
        *((dst)+i) = *((src)+i);\
    }\
} while(0)

internal int32
DecodePacket(AVFrame **frame, AVPacket *pkt, AVCodecContext *codec_context)
{
    StartTimer("DecodePacket()");
    
    int ret = 0;
    ret = avcodec_send_packet(codec_context, pkt);
    
    if(ret == 0)
    {
        av_packet_unref(pkt);
    }
    else if(ret == AVERROR(EAGAIN))
    {
        dbg_error("AVERROR(EAGAIN)\n");
        // TODO(Val): This means a frame must be read before we can send another packet
    }
    else if(ret == AVERROR_EOF)
    {
        // TODO(Val): Mark file as finished
        dbg_error("AVERROR(EOF)\n");
        // NOTE(Val): Decoder flushed
    }
    else if(ret == AVERROR(EINVAL))
    {
        dbg_error("AVERROR(EINVAL)\n");
        RETURN(WRONG_ARGS);
        // NOTE(Val): codec not opened, it is an encoder, or requires flush
    }
    else if(ret == AVERROR(ENOMEM))
    {
        dbg_error("AVERROR(ENOMEM)\n");
        RETURN(NO_MEMORY);
        // NOTE(Val): No memory
    }
    else if(ret < 0)
    {
        dbg_error("Decoding error.\n");
    }
    
    ret = avcodec_receive_frame(codec_context, *frame);
    
    if(ret == AVERROR(EAGAIN))
    {
        // NOTE(Val): This means we need to send more data
        RETURN(NEED_DATA);
    }
    else if(ret == AVERROR_EOF)
    {
        // TODO(Val): Mark file as finished.
        dbg_warn("AVERROR_EOF\n");
        RETURN(FILE_EOF);
    }
    else if(ret == AVERROR(EINVAL))
    {
        dbg_warn("AVERROR(EINVAL)\n");
        // NOTE(Val): codec not opened, it is an encoder, or requires flush
        RETURN(WRONG_ARGS);
    }
    else if(ret < 0)
    {
        dbg_error("Decoding error.\n");
    }
    
    EndTimer();
    RETURN(SUCCESS);
}
