#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "defines.h"
#include "deb-watchtogether-v2.h"
#include "watchtogether.h"

global AVCodec *audio_codec;
global AVCodec *video_codec;
global AVCodecParserContext *parser;
global AVCodecContext *codec_context = NULL;
global AVFormatContext *format_context = NULL;
global AVPacket *pkt;
global AVFrame *frame;
global int video_stream = -1;
global int audio_stream = -1;

static void
video_close()
{
    
    av_parser_close(parser);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

static void
video_encode()
{
    
}

static int32
video_decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;
    
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        
        return 0;
    }
    
    
    while((ret = avcodec_receive_frame(dec_ctx, frame)))
    {
        //printf("Error during decoding\n");
        //printf("%s\n", av_err2str(ret));
        if(ret == 0)
        {
            // NOTE(Val): All good
            // TODO(Val): Do something here?
            return 0;
        }
        else if (ret == AVERROR_EOF) {
            //printf("ret == AVERROR(EAGAIN) || ret == AVERROR_EOF\n");
            fprintf(stderr, "%s\n", av_err2str(ret));
            
            int seek_target= av_rescale_q(0, AV_TIME_BASE_Q,
                                          format_context->streams[video_stream]->time_base);
            
            av_seek_frame(format_context, video_stream, 0, AVSEEK_FLAG_ANY);
            return 0;
        }
        else if (ret == AVERROR(EAGAIN))
        {
            printf("Packet size: %d\n", pkt->size);
            ret = av_read_frame(format_context, pkt);
            
            ret = avcodec_send_packet(dec_ctx, pkt);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                return 0;
            }
        }
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            fprintf(stderr, "%s\n", av_err2str(ret));
            return -1;
        }
        else
        {
            fprintf(stderr, "Error during decoding\n");
            fprintf(stderr, "%s\n", av_err2str(ret));
        }
        
        /* the picture is allocated by the decoder. no need to
           free it */
    }
    //printf("Error during decoding\n");
    //printf("%s\n", av_err2str(ret));
    
    return 0;
}

static uint32
copy_context(AVCodecContext *dst, const AVCodecContext *src)
{
    AVCodecParameters *params = avcodec_parameters_alloc();
    
    int ret = 0;
    
    if((ret = avcodec_parameters_from_context(params, src)) < 0)
    {
        printf("avcodec_parameters_from_context() failed!");
        avcodec_parameters_free(&params);
        return ret;
    }
    
    if((ret = avcodec_parameters_to_context(dst, params)) < 0)
    {
        printf("avcodec_parameters_to_context() failed!");
        avcodec_parameters_free(&params);
        return ret;
    }
    
    avcodec_parameters_free(&params);
    
    return 0;
}

static uint32
video_open(const char *filename)
{
    
    // Open video file
    int ret = 0;
    if((ret = avformat_open_input(&format_context, filename, NULL, NULL)) < 0)
    {
        printf("Error: %d\n", ret);
        return -1; // Couldn't open file
    }
    
    // Retrieve stream information
    if(avformat_find_stream_info(format_context, NULL) < 0)
        return -1; // Couldn't find stream information
    
    // Dump information about file onto standard error
    av_dump_format(format_context, 0, filename, 0);
    
    video_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    audio_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    
    codec_context = avcodec_alloc_context3(video_codec);
    if(!codec_context)
    {
        av_log(NULL, AV_LOG_TRACE, "avcodec_alloc_context3() failed!\n");
        return -1;
    }
    avcodec_parameters_to_context(codec_context, format_context->streams[video_stream]->codecpar);
    //av_log(codec_context, AV_LOG_INFO, NULL);
    
    
    if(avcodec_open2(codec_context, video_codec, NULL) < 0)
    {
        printf("acvodec_open2() failed!\n");
        return -1;
    }
    
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    if (!pkt)
    {
        printf("av_packet_alloc() failed!\n");
        return -1;
    }
    
    parser = av_parser_init(video_codec->id);
    if (!parser) {
        printf("av_parser_init() failed!\n");
        return -1;
    }
    
    frame = av_frame_alloc();
    if (!frame) {
        printf("av_frame_alloc() failed!\n");
        return -1;
    }
    
    //av_log(codec_context, AV_LOG_INFO, "Codec Context\n");
    //av_log(format_context, AV_LOG_INFO, "Format Context\n");
    
    AVRational time;
    time = format_context->streams[video_stream]->avg_frame_rate;
    printf("%d/%d\n", time.num, time.den);
    set_FPS((real32)time.den/(real32)time.num * 1000.0f);
    
    return 0;
}


static void*
video_get_next_frame(pixel_buffer *buffer)
{
    static struct SwsContext* modifContext = NULL;
    
    do {
        if(pkt)
            av_packet_unref(pkt);
        av_read_frame(format_context, pkt);
    } while(!(pkt->size && pkt->stream_index == video_stream));
    
    video_decode(codec_context, frame, pkt);
    av_packet_unref(pkt);
    
    modifContext = sws_getCachedContext(modifContext,
                                        codec_context->width,
                                        codec_context->height,
                                        frame->format,
                                        buffer->width,
                                        buffer->height,
                                        AV_PIX_FMT_RGB32,
                                        SWS_BICUBIC,
                                        NULL, NULL, NULL);
    
    uint8_t *ptrs[1] = { buffer->buffer };
    int stride[] = { buffer->pitch };
    
    /*
    printf("Pointer: %p\nWidth: %d\nHeight: %d\nPitch: %d\n",
           buffer->buffer,
           buffer->width,
           buffer->height,
           buffer->pitch);
    */
    sws_scale(modifContext,
              (uint8_t const* const*)frame->data,
              frame->linesize,
              0,
              codec_context->height,
              (uint8 *const *const)ptrs,
              stride);
    
    return buffer;
}
