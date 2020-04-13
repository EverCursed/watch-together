/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Decoding header. For now it also holds information needing to stream
to a different client. 
*/

#ifndef DECODING
#define DECODING

//#include "watchtogether.h"
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "SDL_platform.h"
#include "defines.h"
#include "packet_queue.h"

typedef struct _decoder_info {
    AVRational video_time_base;
    AVRational audio_time_base;
    AVRational subtitle_time_base;
    
    AVFormatContext *format_context;
    AVFormatContext *output_context;
    //AVFormatContext *output_audio_context;
    //AVFormatContext *output_subtitle_context;
    
    AVCodec *audio_codec;
    AVCodec *video_codec;
    AVCodec *subtitle_codec;
    
    AVCodecContext *audio_codec_context;
    AVCodecContext *video_codec_context;
    AVCodecContext *subtitle_codec_context;
    
    AVCodecContext *output_video_codec_context;
    //AVCodecContext *output_audio_codec_context;
    //AVCodecContext *output_subtitle_codec_context;
    
    AVStream *video_stream;
    AVStream *audio_stream;
    AVStream *subtitle_stream;
    
    AVStream *output_video_stream;
    AVStream *output_audio_stream;
    AVStream *output_subtitle_stream;
    
    i32 video_stream_index;
    i32 audio_stream_index;
    i32 subtitle_stream_index;
    
    i32 stream_mapping_size;
    i32 *stream_mapping;
    
    const char *filename;
    
    f64 avg_video_framerate;
    cond_info condition;
    cond_info start_streaming;
    avpacket_queue *queue;
    i32 frames_sent;
    
    b32 file_fully_loaded;
} decoder_info;


internal i32 DecodingThreadStart(void *);
internal i32 DecodePacket(AVFrame **, AVPacket *, AVCodecContext *);

#endif