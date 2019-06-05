#ifndef DECODING
#define DECODING

typedef struct _decoder_info {
    AVRational video_time_base;
    AVRational audio_time_base;
    
    AVFormatContext *format_context;
    
    AVCodec *audio_codec;
    AVCodec *video_codec;
    
    AVCodecContext *audio_codec_context;
    AVCodecContext *video_codec_context;
    
    int video_stream;
    int audio_stream;
    
    const char *filename;
} decoder_info;

#endif