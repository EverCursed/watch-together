#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "defines.h"
#include "deb-watchtogether-v2.h"



#define INBUF_SIZE 4096

typedef struct _frame_buffer {
    
} frame_buffer;

global FILE *video_file;

global AVCodec *audio_codec;
global AVCodec *video_codec;
global AVCodecParserContext *parser;
global AVCodecContext *codec_context = NULL;
global AVFormatContext *format_context = NULL;
global AVPacket *pkt;
global AVFrame *frame;
global uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
global int video_stream = -1;
global int audio_stream = -1;

static void
video_encode()
{
    
}

static uint32
video_decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
    char buf[1024];
    int ret;
    
    printf("Packet size: %d\n", pkt->size);
    
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        
        return 0;
    }
    
    
    while((ret = avcodec_receive_frame(dec_ctx, frame)))
    {
        printf("Error during decoding\n");
        printf("%s\n", av_err2str(ret));
        
        if (ret == AVERROR_EOF) {
            //printf("ret == AVERROR(EAGAIN) || ret == AVERROR_EOF\n");
            fprintf(stderr, "%s\n", av_err2str(ret));
            
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
            exit(1);
        }
        else
        {
            fprintf(stderr, "Error during decoding\n");
            fprintf(stderr, "%s\n", av_err2str(ret));
        }
        
        /* the picture is allocated by the decoder. no need to
           free it */
    }
    printf("Error during decoding\n");
    printf("%s\n", av_err2str(ret));
    
}

static uint32
copy_context(AVCodecContext *dst, const AVCodecContext *src)
{
    AVCodecParameters *params = avcodec_parameters_alloc();
    
    int ret = 0;
    
    if((ret = avcodec_parameters_from_context(params, src)) < 0)
    {
        printf("avcodec_parameters_from_context() failed!");
        return ret;
    }
    
    if((ret = avcodec_parameters_to_context(dst, params)) < 0)
    {
        printf("avcodec_parameters_to_context() failed!");
        return ret;
    }
    
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
    else
        printf("av_format_open_input() succeeded!\n");
    
    // Retrieve stream information
    if(avformat_find_stream_info(format_context, NULL) < 0)
        return -1; // Couldn't find stream information
    else
        printf("avformat_find_stream_info() succeeded!\n");
    
    // Dump information about file onto standard error
    av_dump_format(format_context, 0, filename, 0);
    
    /*
    // Find the first video stream
    int videoStream = -1;
    for(int i = 0; i < format_context->nb_streams; i++)
        if(format_context->video_codec_id==AVMEDIA_TYPE_VIDEO) {
        videoStream = i;
        break;
    }
    if(videoStream == -1)
        return -1; // Didn't find a video stream
    */
    // Get a pointer to the codec context for the video stream
    
    
    // codec_context = 
    video_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    audio_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    
    //avcodec_parameters_to_context(codec_context, format_context->streams[video_stream]->codecpar);
    /*
        codec = avcodec_find_decoder(format_context->streams[video_stream]->codecpar->codec_id);
    if(!codec)
        printf("avcodec_find_decoder() failed!\n");
    else
        printf("avcodec_find_decoder() succeeded!\n");
    */
    
    codec_context = avcodec_alloc_context3(video_codec);
    if(!codec_context)
    {
        av_log(NULL, AV_LOG_TRACE, "avcodec_alloc_context3() failed!\n");
        return -1;
    }
    else
    {
        printf("avcodec_alloc_context3() succeeded!\n");
        avcodec_parameters_to_context(codec_context, format_context->streams[video_stream]->codecpar);
        av_log(codec_context, AV_LOG_INFO, NULL);
    }
    
    if(avcodec_open2(codec_context, video_codec, NULL) < 0)
    {
        printf("acvodec_open2() failed!\n");
    }
    else
    {
        printf("acvodec_open2() succeeded!\n");
    }
    
    pkt = av_packet_alloc();
    //av_init_packet(pkt);
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
    
    /*
// set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) 
memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
*/
    
    av_log(codec_context, AV_LOG_INFO, "Codec Context\n");
    av_log(format_context, AV_LOG_INFO, "Format Context\n");
    
    
    return 0;
}

static void
video_close()
{
    /* flush the decoder */
    //video_decode(codec_context, frame, NULL);
    
    //fclose(video_file);
    
    av_parser_close(parser);
    avcodec_free_context(&codec_context);
    //avcodec_free_context(&codec_context_copy);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

static void
video_get_next_frame(AVFrame *frame)
{
    uint8_t *data;
    size_t data_size;
    int ret;
    
    /* read raw data from the input file */
    //data_size = fread(inbuf, 1, INBUF_SIZE, video_file);
    //if (!data_size)
    //return;
    
    ret = av_read_frame(format_context, pkt);
    if (pkt->size && pkt->stream_index == video_stream)
        video_decode(codec_context, frame, pkt);
    
    //blit_frame(frame);
    
    /*
     // use the parser to split the data into frames 
    data = inbuf;
    while (data_size > 0) {
        ret = av_parser_parse2(parser, codec_context, &pkt->data, &pkt->size,
                               data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        else
        {
            printf("%d\n", pkt->size);
        }
        data      += ret;
        data_size -= ret;
        }
    */
}
