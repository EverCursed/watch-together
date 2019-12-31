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
    
    int32 video_stream_index;
    int32 audio_stream_index;
    int32 subtitle_stream_index;
    
    int32 stream_mapping_size;
    int32 *stream_mapping;
    
    const char *filename;
    
    real64 avg_video_framerate;
    cond_info condition;
    cond_info start_streaming;
    avpacket_queue *queue;
    int32 frames_sent;
    
    bool32 file_fully_loaded;
} decoder_info;


int32 DecodingThreadStart(void *);
int32 DecodePacket(AVFrame **, AVPacket *, AVCodecContext *);

#endif