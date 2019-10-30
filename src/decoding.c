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

int32
get_frame(avpacket_queue *queue, AVFrame **frame, AVCodecContext *dec_ctx)
{
    StartTimer("get_frame()");
    
    int ret = 0;
    
    AVPacket *pkt = NULL;// = av_packet_alloc();
    
    *frame = av_frame_alloc();
    if(!frame)
        RETURN(NO_MEMORY);
    
    do {
        if(f(peek_packet(queue, &pkt, 0)))
        {
            dbg_error("There were no packets in queue.\n");
            av_frame_free(frame);
            RETURN(NOT_ENOUGH_DATA);
        }
        
        ret = avcodec_send_packet(dec_ctx, pkt);
        
        if(ret == 0)
        {
            av_packet_unref(pkt);
            dequeue_packet(queue, &pkt);
        }
        else if(ret == AVERROR(EAGAIN))
        {
            dbg_error("AVERROR(EAGAIN)\n");
            // TODO(Val): This means a frame must be read before we can send another packet
        }
        else if(ret == AVERROR_EOF)
        {
            //pdata->file.file_finished = 1;
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
        
        ret = avcodec_receive_frame(dec_ctx, *frame);
        //dbg_info("avcodec_receive_frame\tret: %d\n", ret);
        
        if(ret == AVERROR(EAGAIN))
        {
            continue;
            // NOTE(Val): This means a frame must be read before we can send another packet
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
    } while(ret == AVERROR(EAGAIN)); // && pdata->running
    
    EndTimer();
    RETURN(SUCCESS);
}

#define AUDIO_FRAME 1
#define VIDEO_FRAME 2

static inline void
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

AVFrame *
DecodePacket(avpacket_queue *queue, AVCodecContext *codec_context)
{
    AVFrame *frame = NULL;
    get_frame(queue, &frame, codec_context);
    
    return frame;
}
