/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Encoding module. Currently unused.
*/

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>

//#include "decoding.h"
#include "encoding.h"
#include "file_data.h"
#include "defines.h"
//#include "media_processing.h"

#define BIT_RATE 400000

#define PRESET "ultrafast"
#define TUNE   "zerolatency"

static int codec_id = AV_CODEC_ID_H264;
static AVCodec *codec;
static AVCodecContext *c = NULL;

int32 i, ret, x, y, got_output;

void
init_encoding(open_file_info *file)
{
    codec = avcodec_find_encoder(codec_id);
    c = avcodec_alloc_context3(codec);
    
    c->bit_rate = BIT_RATE;
    c->width = file->width;
    c->height = file->height;
    c->time_base = av_d2q(file->target_time, 5000); // TODO(Val): change this so we don't need to guess
    c->gop_size = (int)(c->time_base.den/c->time_base.num);
    c->max_b_frames = 1; // TODO(Val): Not sure what this is so just leaving for now
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    //c->flags = CODEC_FLAG_GLOBAL_HEADER;
    
    if (codec_id == AV_CODEC_ID_H264) {
        ret = av_opt_set(c->priv_data, "preset", PRESET, 0);
        ret = av_opt_set(c->priv_data, "tune", TUNE, 0);
    }
    
    avcodec_open2(c, codec, NULL);
}

/*
// extracted
{

    struct AVStream* stream = avformat_new_stream(avfctx, codec);
    stream->codecpar->bit_rate = 400000;
    stream->codecpar->width = 352;
    stream->codecpar->height = 288;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->time_base.num = 1;
    stream->time_base.den = 25;
    
    
    AVOutputFormat* fmt = av_guess_format("rtp", NULL, NULL);
    ret = avformat_alloc_output_context2(&avfctx, fmt, fmt->name,
                                         "rtp://127.0.0.1:23777");
                                         
                                         
    avio_open(&avfctx->pb, , AVIO_FLAG_WRITE);
    
    avformat_write_header(avfctx, NULL);
    char buf[200000];
    AVFormatContext *ac[] = { avfctx };
    //av_sdp_create(ac, 1, buf, 20000);
    //printf("sdp:\n%s\n", buf);
    //FILE* fsdp;
    //fopen_s(&fsdp, "test.sdp", "w");
    //fprintf(fsdp, "%s", buf);
    //fclose(fsdp);
    
    
    if (ret == AVERROR_EOF) {
        got_output = false;
        printf("Stream EOF\n");
    } else if(ret == AVERROR(EAGAIN)) {
        got_output = false;
        printf("Stream EAGAIN\n");
    } else {
        got_output = true;
    }
    
    if (got_output) {
        printf("Write frame %3d (size=%5d)\n", j++, pkt.size);
        av_interleaved_write_frame(avfctx, &pkt);
        av_packet_unref(&pkt);
    }
    
    
        if (got_output) {
            //printf("Write frame %3d (size=%5d)\n", j++, pkt.size);
            av_interleaved_write_frame(avfctx, &pkt);
            av_packet_unref(&pkt);
        }
        
    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    printf("\n");
    
}
*/

AVPacket
encode_video_frame(AVFrame *frame)
{
    // TODO(Val): Make sure this is a video frame
    //int got_output = 0;
    
    //AVFormatContext* avfctx;
    AVPacket pkt;
    
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;
    
    ret = avcodec_send_frame(c, frame);
    ret = avcodec_receive_packet(c, &pkt);
    
    /* get the delayed frames */
    for (; ; i++) {
        fflush(stdout);
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR_EOF) {
            printf("Stream EOF\n");
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            printf("Stream EAGAIN\n");
            //got_output = 0;
        } else {
            //got_output = 1;
        }
    }
    
    return pkt;
}