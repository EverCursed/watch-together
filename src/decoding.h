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
    
    AVFormatContext *format_context;
    
    AVCodec *audio_codec;
    AVCodec *video_codec;
    
    AVCodecContext *audio_codec_context;
    AVCodecContext *video_codec_context;
    
    int32 video_stream;
    int32 audio_stream;
    int32 subtitle_stream;
    
    const char *filename;
    
    real64 avg_video_framerate;
    cond_info condition;
} decoder_info;


int32 DecodingThreadStart(void *);
int32 DecodePacket(AVFrame **, AVPacket *, AVCodecContext *);

#endif