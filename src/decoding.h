#ifndef DECODING
#define DECODING

#include "defines.h"

typedef struct _open_file_info {
    char *filename;
    
    uint32 width;
    uint32 height;
    uint32 video_format;
    real32 fps;
    real32 target_time;
    
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
    
    bool32 has_audio;
    bool32 has_video;
    bool32 has_subtitles;
    
    bool32 file_opened;        // signals that the file was successfully opened
    bool32 file_ready;         // // TODO(Val): signals that the data is ready to be displayed
    bool32 open_failed;        // signals that opening the file failed
    
    volatile bool32 file_finished;
    // TODO(Val): audio format
} open_file_info;

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