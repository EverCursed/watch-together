#ifndef DECODING
#define DECODING

#include "defines.h"


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
    
    const char *filename;
    
    cond_info condition;
} decoder_info;


int32
DecodingFileOpen(open_file_info *file, decoder_info *decoder);

void
DecodingFileClose(open_file_info *file, decoder_info *decoder);

int32 
DecodingThreadStart(void *ptr);

#endif