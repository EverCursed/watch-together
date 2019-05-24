#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "defines.h"
#include "watchtogether.h"

global AVCodec *audio_codec;
global AVCodec *video_codec;
global AVCodecContext *video_codec_context = NULL;
global AVCodecContext *audio_codec_context = NULL;
global AVFormatContext *format_context = NULL;
//global AVPacket *pkt;
//global AVFrame *frame;
global int video_stream = -1;
global int audio_stream = -1;

static void
file_close()
{
    avcodec_free_context(&video_codec_context);
    avformat_close_input(&format_context);
    //av_frame_free(&frame);
    //av_packet_unref(pkt);
}

#define FRAME_AUDIO 1
#define FRAME_VIDEO_RGB 2
#define FRAME_VIDEO_YUV 3
#define FRAME_UNKNOWN -1
struct frame_info {
    AVFrame *frame;
    int32 type;
};

// returns an AVFrame. if the frame var is NULL,
// then something went wrong
static int32
wt_decode(struct frame_info *info)
{
    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
    {
        dbg_error("av_packet_alloc() failed!\n");
        return -1;
    }
    
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        dbg_error("av_frame_alloc() failed!\n");
        return -1;
    }
    
    int32 ret = 0;
    
    AVCodecContext *dec_ctx = NULL;
    
    ret = av_read_frame(format_context, pkt);
    if(ret < 0)
    {
        dbg_error("End of file.\n");
        return -1;
    }
    
    // set the type of frame.
    if(pkt->stream_index == video_stream)
    {
        //printf("frame->format == %d\n", format_context->streams[video_stream]->codecpar->format);
        if(format_context->streams[video_stream]->codecpar->format == AV_PIX_FMT_YUV420P)
            //frame->format == )
        {
            info->type = FRAME_VIDEO_YUV;
        }
        else
        {
            info->type = FRAME_VIDEO_RGB;
        }
        
        dec_ctx = video_codec_context;
    }
    else if(pkt->stream_index == audio_stream)
    {
        info->type = FRAME_AUDIO;
        dec_ctx = audio_codec_context;
    }
    else
    {
        info->type = FRAME_UNKNOWN;
        return -1;
    }
    
    receive_packet:
    ret = avcodec_send_packet(dec_ctx, pkt);
    if(ret == AVERROR(EAGAIN)) goto receive_packet;
    else if (ret == AVERROR_EOF ||
             ret == AVERROR(EINVAL) ||
             ret == AVERROR(ENOMEM)) 
    {
        dbg_error("Error sending a packet for decoding\n");
        return -1;
    }
    
    ret = avcodec_receive_frame(dec_ctx, frame);
    if(ret == AVERROR(EAGAIN)) goto receive_packet;
    
    if(ret == AVERROR(EINVAL))
    {
        dbg_error("AVERROR(EINVAL)\n");
        return -1;
    }
    else if(ret == AVERROR_EOF)
    {
        dbg_error("AVERROR_EOF\n");
        // TODO(Val): End of file, do something here.
        return -1;
    }
    else 
    {
        info->frame = frame;
    }
    
    av_packet_unref(pkt);
    return 0;
}

/*
struct decoder_properties {
uint32 audio_frequency;
uint32 audio_channels;
uint32 audio_bytes_per_sample;
uint32 video_width;
uint32 video_height;
uint32 video_bytes_per_sample;
};
*/

static int32
file_open(open_file_info *file)
{
    // Open video file
    int ret = 0;
    if(!(ret = avformat_open_input(&format_context, file->filename, NULL, NULL)) < 0)
    {
        dbg_error("AV open input failed.\n");
        return -1; // Couldn't open file
    }
    
    // Retrieve stream information
    //format_context = avformat_alloc_context();
    if(avformat_find_stream_info(format_context, NULL) < 0)
    {
        dbg_error("Couldn't find stream info\n");
        return -1; // Couldn't find stream information
    }
    // Dump information about file onto standard error
    av_dump_format(format_context, 0, file->filename, 0);
    
    // Query stream indexes from opened file
    video_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
    audio_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    
    if(video_stream >= 0)
    {
        dbg_success("av_find_best_steam successful for video.\n");
    }
    else if(video_stream == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("AVERROR_STREAM_NOT_FOUND\n");
    }
    else if(video_stream == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    if(audio_stream >= 0)
    {
        dbg_success("av_find_best_steam successful for audio.\n");
    }
    else if(audio_stream == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("AVERROR_STREAM_NOT_FOUND\n");
    }
    else if(audio_stream == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    // Init video codec context
    if(video_stream >= 0)
    {
        video_codec_context = avcodec_alloc_context3(video_codec);
        if(!video_codec_context)
        {
            dbg_error("Video: avcodec_alloc_context3() failed!\n");
            return -1;
        }
        avcodec_parameters_to_context(video_codec_context, format_context->streams[video_stream]->codecpar);
        
        if(avcodec_open2(video_codec_context, video_codec, NULL) < 0)
        {
            dbg_error("Video: avcodec_open2() failed!\n");
            return -1;
        }
        
        if(format_context->streams[video_stream]->codecpar->format == AV_PIX_FMT_YUV420P)
        {
            file->video_format = VIDEO_YUV;
        }
        else //if(format_context->streams[video_stream]->codecpar->format == AV_PIX_FMT_RGB32)
        {
            file->video_format = VIDEO_RGB;
        }
        
        
        file->width = video_codec_context->width;
        file->height = video_codec_context->height;
        
        AVRational time;
        time = format_context->streams[video_stream]->avg_frame_rate;
        file->fps = (real32)time.num/(real32)time.den;
        file->target_time = (real32)time.den/(real32)time.num * 1000.0f;
        
        file->has_video = 1;
    }
    
    if(audio_stream >= 0)
    {
        file->has_audio = 1;
        
        // Init audio codec context
        audio_codec_context = avcodec_alloc_context3(audio_codec);
        if(!audio_codec_context)
        {
            dbg_error("Audio: avcodec_alloc_context3() failed!\n");
            return -1;
        }
        avcodec_parameters_to_context(audio_codec_context, format_context->streams[audio_stream]->codecpar);
        
        if(avcodec_open2(audio_codec_context, audio_codec, NULL) < 0)
        {
            dbg_error("Audio: avcodec_open2() failed!\n");
            return -1;
        }
        
        uint32 sample_fmt = audio_codec_context->sample_fmt;
        uint32 fmt = 0;
        uint32 bytes_per_sample = 0;
        if(sample_fmt == AV_SAMPLE_FMT_U8 ||
           sample_fmt == AV_SAMPLE_FMT_U8P)
        {
            fmt = SAMPLE_U8;
            bytes_per_sample = 1;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S16 ||
                sample_fmt == AV_SAMPLE_FMT_S16P)
        {
            fmt = SAMPLE_S16;
            bytes_per_sample = 2;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S32 ||
                sample_fmt == AV_SAMPLE_FMT_S32P)
        {
            fmt = SAMPLE_S32;
            bytes_per_sample = 4;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S64)
        {
            fmt = SAMPLE_S64;
            bytes_per_sample = 8;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_FLT ||
                sample_fmt == AV_SAMPLE_FMT_FLTP)
        {
            fmt = SAMPLE_FLOAT;
            bytes_per_sample = 4;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_DBL ||
                sample_fmt == AV_SAMPLE_FMT_DBLP)
        {
            fmt = SAMPLE_DOUBLE;
            bytes_per_sample = 8;
        }
        else
        {
            dbg_error("Unhandled audio format\n");
            return -1;
        }
        
        //dbg_print("bytes_per_sample: %d\n", bytes_per_sample);
        
        file->sample_rate = audio_codec_context->sample_rate;
        file->bytes_per_sample = bytes_per_sample;
        file->channels = audio_codec_context->channels;
        file->sample_format = fmt;
    }
    
    return 0;
}

#define AUDIO_FRAME 1
#define VIDEO_FRAME 2

struct frame {
    void *buffer;
    uint32 size;
    uint32 type;
};

global struct SwsContext* modifContext = NULL;

static int32
process_frame(struct frame_info info, program_data *pdata)
{
    uint32 ret = 0;
    AVFrame *frame = info.frame;
    
    if(pdata->running && info.type == FRAME_VIDEO_RGB)
    {
        modifContext = sws_getCachedContext(modifContext,
                                            video_codec_context->width,
                                            video_codec_context->height,
                                            frame->format,
                                            video_codec_context->width,  // dst width
                                            video_codec_context->height, // dst height
                                            //video_buffer->width,
                                            //video_buffer->height,
                                            AV_PIX_FMT_RGB32,
                                            SWS_BICUBIC,
                                            NULL, NULL, NULL);
        
        void *temp_buffer = malloc(pdata->vq_data.video_queue_frame_size);
        uint8_t *ptrs[1] = { temp_buffer };
        int stride[1] = { pdata->vq_data.video_queue_pitch };
        
        sws_scale(modifContext,
                  (uint8 const* const*)frame->data,
                  frame->linesize,
                  0,
                  video_codec_context->height,
                  (uint8 *const *const)ptrs,
                  stride);
        
        while(pdata->running && (enqueue_frame(&pdata->vq_data, temp_buffer, pdata->vq_data.video_queue_pitch) < 0))
        {
            SDL_Delay((int32)pdata->file.target_time);
        }
        free(temp_buffer);
    }
    else if(pdata->running && info.type  == FRAME_VIDEO_YUV)
    {
        dbg_print("linesize[0] = %d\n"
                  "linesize[1] = %d\n"
                  "linesize[2] = %d\n"
                  "width = %d\n"
                  "height = %d\n",
                  frame->linesize[0],
                  frame->linesize[1],
                  frame->linesize[2],
                  frame->width,
                  frame->height);
        while(pdata->running && (enqueue_frame_YUV(&pdata->vq_data,
                                                   frame->data[0],
                                                   frame->linesize[0],
                                                   frame->data[1],
                                                   frame->linesize[1],
                                                   frame->data[2],
                                                   frame->linesize[2])))
        {
            SDL_Delay((int32)pdata->file.target_time);
        }
    }
    else if(info.type == FRAME_AUDIO)
    {
        uint32 size = av_samples_get_buffer_size(NULL, 
                                                 audio_codec_context->channels,
                                                 frame->nb_samples,
                                                 audio_codec_context->sample_fmt,
                                                 1);
        //dbg_print("Audio buffer size: %d\n", size);
        
        uint32 real_size = frame->linesize[0] * audio_codec_context->channels; 
        //dbg_print("Size should be: %d\n", real_size);
        
        uint32 SampleCount = frame->nb_samples;
        uint32 Frequency = audio_codec_context->sample_rate;
        uint32 Channels = audio_codec_context->channels;
        //dbg_print("Linesize: %d - %d\n", frame->linesize[0], size);
        //dbg_print("Channels: %d\n", Channels);
        /*
        AV_SAMPLE_FMT_U8 	unsigned 8 bits
        AV_SAMPLE_FMT_S16 	signed 16 bits
        AV_SAMPLE_FMT_S32 	signed 32 bits
        AV_SAMPLE_FMT_S64 	signed 64 bits
        AV_SAMPLE_FMT_FLT 	float
        AV_SAMPLE_FMT_DBL 	double
        AV_SAMPLE_FMT_U8P 	unsigned 8 bits, planar
        AV_SAMPLE_FMT_S16P 	signed 16 bits, planar
        AV_SAMPLE_FMT_S32P 	signed 32 bits, planar
        AV_SAMPLE_FMT_FLTP 	float, planar
        AV_SAMPLE_FMT_DBLP 	double, planar
        AV_SAMPLE_FMT_S64P 
        */
        bool32 is_planar = 0;
        uint32 sample_fmt = audio_codec_context->sample_fmt; 
        uint32 bytes_per_sample = frame->linesize[0]/frame->nb_samples;
        
        void *data = malloc(size*2);
        
        if(sample_fmt == AV_SAMPLE_FMT_U8P  ||
           sample_fmt == AV_SAMPLE_FMT_S16P ||
           sample_fmt == AV_SAMPLE_FMT_S32P ||
           sample_fmt == AV_SAMPLE_FMT_S64P ||
           sample_fmt == AV_SAMPLE_FMT_FLTP ||
           sample_fmt == AV_SAMPLE_FMT_DBLP)
        {
            is_planar = 1;
        }
        
        if(!is_planar)
            memcpy(data, frame->data[0], size);
        else
        {
            // NOTE(Val): manually interleaving audio for however
            // many channels
            uint8* dst = data;
            uint32 channels = audio_codec_context->channels;
            uint32 length = frame->nb_samples; // in samples
            
            for(int s = 0; s < length; s++)
            {
                for(int c = 0; c < channels; c++)
                {
                    for(int b = 0; b < bytes_per_sample; b++)
                    {
                        *(dst +
                          s*channels*bytes_per_sample +
                          bytes_per_sample*c + b) = *(frame->data[c] + s*bytes_per_sample + b);
                    }
                }
            }
        }
        
        while(pdata->running && (enqueue_audio_bytes(&pdata->aq_data, data, size) < 0))
        {
            PlatformSleep(5);
        }
        free(data);
    }
}

static int32 
DecodingThreadStart(void *ptr)
{
    program_data *pdata = ptr;
    open_file_info *file = &pdata->file;
    if(!file_open(file))
    {
        dbg_success("File opened successfully.\n");
        PlatformPauseAudio(0);
        
        if(file->has_video)
        {
            if(file->video_format == VIDEO_YUV)
            {
                dbg_info("Initializing YUV queue.\n");
                init_video_queue_YUV(&pdata->vq_data,
                                     pdata->file.width,
                                     pdata->file.height);
            }
            else
            {
                dbg_info("Initializing RGB queue.\n");
                reset_video_queue(&pdata->vq_data,
                                  pdata->file.width,
                                  pdata->file.height,
                                  4);
            }
        }
        
        if(file->has_audio)
        {
            reset_audio_queue(&pdata->aq_data,
                              pdata->file.sample_rate,
                              pdata->file.channels,
                              pdata->file.bytes_per_sample);
        }
        
        dbg_success("Audio and video queues initialized.\n");
        
        if(pdata->file.has_audio)
        {
            dbg_info("has_audio is set.\n");
            PlatformInitAudio(pdata);
        }
        if(pdata->file.has_video)
        {
            PlatformInitVideo(pdata);
        }
        
        pdata->playing = 1;
        pdata->paused = 0;
        file->file_ready = 1;
        
        struct frame_info info = {};
        while(pdata->running)
        {
            int ret = wt_decode(&info);
            
            if(!ret)
            {
                if(info.type != FRAME_UNKNOWN &&
                   info.frame)
                {
                    process_frame(info, pdata);
                    
                    av_frame_free(&info.frame);
                }
            }
        }
    }
    else
    {
        dbg_error("Could not open file.\n");
        return -1;
    }
    
    dbg_error("Decoder finished.\n");
    return 0;
}
