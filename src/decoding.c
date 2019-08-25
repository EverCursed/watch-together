#include <libswscale/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

//#include "defines.h"
#include "watchtogether.h"
//#include "platform.h"
//#include "packet_queue.h"
//#include "decoding.h"


#define FRAME_AUDIO 1
//#define FRAME_VIDEO_RGB 2
//#define FRAME_VIDEO_YUV 3
#define FRAME_VIDEO 4
#define FRAME_UNKNOWN -1


// TODO(Val): Make this more neat 
struct frame_info {
    AVFrame *frame;
    int32 type;
    int32 ret;
};

int32
get_packet(program_data *pdata, AVPacket *packet)
{
    int ret = av_read_frame(pdata->decoder.format_context, packet);
    
    if(ret == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("Stream not found.\n");
        return -1;
    }
    else if(ret == AVERROR_EOF)
    {
        // TODO(Val): Mark that there are no more packets for this file. 
        pdata->file.file_finished = 1;
        
        dbg_error("End of file.\n");
        return -1;
    }
    else if(ret != 0)
    {
        dbg_error("Some other error happened.\n");
        return -1;
    }
    
    return 0;
}

struct frame_info
get_frame(program_data *pdata, avpacket_queue *queue)
{
    decoder_info *decoder = &pdata->decoder;
    AVCodecContext *dec_ctx;
    
    AVPacket *pkt = NULL;// = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    struct frame_info info = {.frame = frame, .type = 0, .ret = -1};
    
    if(!frame) {
        dbg_error("av_frame_alloc() failed!\n");
        return info;
    }
    
    int32 ret = 0;
    
    // set the type of frame.
    if(queue == pdata->pq_video)
    {
        // TODO(Val): Temporarily converting everything to YUV
        info.type = FRAME_VIDEO;
        dec_ctx = decoder->video_codec_context;
    }
    else if(queue == pdata->pq_audio)
    {
        info.type = FRAME_AUDIO;
        dec_ctx = decoder->audio_codec_context;
    }
    else
    {
        dbg_error("Unknown frame returned.\n");
        info.type = FRAME_UNKNOWN;
        goto no_packet_fail;
    }
    
    do {
        ret = peek_packet(queue, &pkt, 0);
        if(ret == -1)
            goto no_packet_fail;
        
        ret = avcodec_send_packet(dec_ctx, pkt);
        //dbg_info("avcodec_send_packet\tret: %d\n", ret);
        
        if(ret == AVERROR(EAGAIN))
        {
            dbg_error("AVERROR(EAGAIN)\n");
            // TODO(Val): This means a frame must be read before we can send another packet
        }
        else if(ret == AVERROR_EOF)
        {
            //pdata->file.file_finished = 1;
            // TODO(Val): Mark file as finished
            dbg_error("AVERROR(EOF)\n");
            // NOTE(Val): Decoder flushed
        }
        else if(ret == AVERROR(EINVAL))
        {
            dbg_error("AVERROR(EINVAL)\n");
            goto get_frame_failed;
            // NOTE(Val): codec not opened, it is an encoder, or requires flush
        }
        else if(ret == AVERROR(ENOMEM))
        {
            dbg_error("AVERROR(ENOMEM)\n");
            // NOTE(Val): No memory
        }
        else if(ret < 0)
        {
            dbg_error("Decoding error.\n");
        }
        else if(ret == 0)
        {
            dequeue_packet(queue, &pkt);
        }
        
        ret = avcodec_receive_frame(dec_ctx, frame);
        //dbg_info("avcodec_receive_frame\tret: %d\n", ret);
        
        if(ret == AVERROR(EAGAIN))
        {
            continue;
            // NOTE(Val): This means a frame must be read before we can send another packet
        }
        //av_packet_unref(pkt);
        else if(ret == AVERROR_EOF)
        {
            // TODO(Val): Mark file as finished.
            dbg_warn("AVERROR_EOF\n");
            goto get_frame_failed;
        }
        else if(ret == AVERROR(EINVAL))
        {
            dbg_warn("AVERROR(EINVAL)\n");
            // NOTE(Val): codec not opened, it is an encoder, or requires flush
            goto get_frame_failed;
        }
        else if(ret < 0)
        {
            dbg_error("Decoding error.\n");
        }
    } while(ret == AVERROR(EAGAIN) && pdata->running);
    
    //av_packet_unref(pkt);
    info.ret = 0;
    //dbg_success("Returning from decoding.\n");
    return info;
    
    get_frame_failed:
    if(pkt)
        av_packet_unref(pkt);
    info.ret = -1;
    return info;
    
    no_packet_fail:
    dbg_error("There were no packets in queue.\n");
    av_frame_free(&frame);
    info.ret = -1;
    return info;
}

int32
DecodingFileOpen(open_file_info *file, decoder_info *decoder)
{
    // Open video file
    dbg_print("avformat version: %d - %d\n", LIBAVFORMAT_VERSION_INT, avformat_version());
    
    if(avformat_open_input(&decoder->format_context, file->filename, NULL, NULL) < 0)
    {
        dbg_error("AV open input failed.\n");
        return -1; // Couldn't open file
    }
    
    // Retrieve stream information
    if(avformat_find_stream_info(decoder->format_context, NULL) < 0)
    {
        dbg_error("Couldn't find stream info\n");
        return -1; // Couldn't find stream information
    }
    // Dump information about file onto standard error
    av_dump_format(decoder->format_context, 0, file->filename, 0);
    
    // Query stream indexes from opened file
    decoder->video_stream = av_find_best_stream(decoder->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder->video_codec, 0);
    decoder->audio_stream = av_find_best_stream(decoder->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder->audio_codec, 0);
    
    if(decoder->video_stream >= 0)
    {
        dbg_success("av_find_best_steam successful for video.\n");
    }
    else if(decoder->video_stream == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("AVERROR_STREAM_NOT_FOUND\n");
    }
    else if(decoder->video_stream == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    if(decoder->audio_stream >= 0)
    {
        dbg_success("av_find_best_steam successful for audio.\n");
    }
    else if(decoder->audio_stream == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("AVERROR_STREAM_NOT_FOUND\n");
    }
    else if(decoder->audio_stream == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    // Init video codec context
    if(decoder->video_stream >= 0)
    {
        decoder->video_codec_context = avcodec_alloc_context3(decoder->video_codec);
        if(!decoder->video_codec_context)
        {
            dbg_error("Video: avcodec_alloc_context3() failed!\n");
            return -1;
        }
        avcodec_parameters_to_context(decoder->video_codec_context, decoder->format_context->streams[decoder->video_stream]->codecpar);
        
        if(avcodec_open2(decoder->video_codec_context, decoder->video_codec, NULL) < 0)
        {
            dbg_error("Video: avcodec_open2() failed!\n");
            return -1;
        }
        
        if(decoder->format_context->streams[decoder->video_stream]->codecpar->format == AV_PIX_FMT_YUV420P)
        {
            file->video_format = VIDEO_YUV;
        }
        else //if(format_context->streams[video_stream]->codecpar->format == AV_PIX_FMT_RGB32)
        {
            file->video_format = VIDEO_YUV;
        }
        
        file->width = decoder->video_codec_context->width;
        file->height = decoder->video_codec_context->height;
        
        
        AVRational time = decoder->format_context->streams[decoder->video_stream]->avg_frame_rate;
        file->fps = (real32)time.num/(real32)time.den;
        file->target_time = (real32)time.den/(real32)time.num * 1.0f;
        
        file->has_video = 1;
        
        decoder->video_time_base = //av_inv_q(
            decoder->format_context->streams[decoder->video_stream]->time_base;
        
        
    }
    
    if(decoder->audio_stream >= 0)
    {
        
        // Init audio codec context
        decoder->audio_codec_context = avcodec_alloc_context3(decoder->audio_codec);
        if(!decoder->audio_codec_context)
        {
            dbg_error("Audio: avcodec_alloc_context3() failed!\n");
            return -1;
        }
        avcodec_parameters_to_context(decoder->audio_codec_context, decoder->format_context->streams[decoder->audio_stream]->codecpar);
        
        if(avcodec_open2(decoder->audio_codec_context, decoder->audio_codec, NULL) < 0)
        {
            dbg_error("Audio: avcodec_open2() failed!\n");
            return -1;
        }
        
        uint32 sample_fmt = decoder->audio_codec_context->sample_fmt;
        uint32 fmt = 0;
        if(sample_fmt == AV_SAMPLE_FMT_U8 ||
           sample_fmt == AV_SAMPLE_FMT_U8P)
        {
            fmt = SAMPLE_U8;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S16 ||
                sample_fmt == AV_SAMPLE_FMT_S16P)
        {
            fmt = SAMPLE_S16;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S32 ||
                sample_fmt == AV_SAMPLE_FMT_S32P)
        {
            fmt = SAMPLE_S32;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_S64)
        {
            fmt = SAMPLE_S64;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_FLT ||
                sample_fmt == AV_SAMPLE_FMT_FLTP)
        {
            fmt = SAMPLE_FLOAT;
        }
        else if(sample_fmt == AV_SAMPLE_FMT_DBL ||
                sample_fmt == AV_SAMPLE_FMT_DBLP)
        {
            fmt = SAMPLE_DOUBLE;
        }
        else
        {
            dbg_error("Unhandled audio format\n");
            return -1;
        }
        
        file->sample_rate = decoder->audio_codec_context->sample_rate;
        file->bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
        file->channels = decoder->audio_codec_context->channels;
        file->sample_format = fmt;
        
        file->has_audio = 1;
        
        decoder->audio_time_base = av_inv_q(decoder->format_context->streams[decoder->audio_stream]->avg_frame_rate);
    }
    
    return 0;
}

void
DecodingFileClose(open_file_info *file, decoder_info *decoder)
{
    // TODO(Val): Should we also stop running the current file?
    
    avformat_free_context(decoder->format_context);
    
    avcodec_free_context(&decoder->audio_codec_context);
    avcodec_free_context(&decoder->video_codec_context);
    
    decoder->audio_codec = NULL;
    decoder->video_codec = NULL;
    
    memset(file, 0, sizeof(open_file_info));
    memset(decoder, 0, sizeof(decoder_info));
}

#define AUDIO_FRAME 1
#define VIDEO_FRAME 2

static inline void
copy_pixel_buffers(uint8 *dst,
                   int32 pitch_dst,
                   uint8 *src,
                   int32 pitch_src,
                   int32 height)
{
    int32 width = pitch_src < pitch_dst ? pitch_src : pitch_dst;
    for(int i = 0; i < height; i++)
    {
        memcpy(dst + i*pitch_dst, src + i*pitch_src, width);
    }
}

global struct SwsContext* modifContext = NULL;

int32
process_video_frame(program_data *pdata, struct frame_info info)
{
    AVFrame *frame = info.frame;
    decoder_info *decoder = &pdata->decoder;
    
    int32 fmt = AV_PIX_FMT_YUV420P;
    
    if(frame->format != AV_PIX_FMT_YUV420P)
    {
        modifContext = sws_getCachedContext(modifContext,
                                            decoder->video_codec_context->width,
                                            decoder->video_codec_context->height,
                                            frame->format,
                                            pdata->video.width,  // dst width
                                            pdata->video.height, // dst height
                                            fmt,
                                            SWS_BICUBIC, //SWS_BILINEAR | SWS_ACCURATE_RND,
                                            NULL, NULL, NULL);
        
        uint8_t *ptrs[3] = {
            pdata->video.video_frame,//frame_Y,
            pdata->video.video_frame_sup1,//frame_U,
            pdata->video.video_frame_sup2,//frame_V
        };
        
        int stride[3] = {
            pdata->video.pitch, //pitch_Y,
            pdata->video.pitch_sup1, //pitch_U,
            pdata->video.pitch_sup2, //pitch_V
        };
        
        sws_scale(modifContext,
                  (uint8 const* const*)frame->data,
                  frame->linesize,
                  0,
                  decoder->video_codec_context->height,
                  (uint8* const* const)ptrs,
                  stride);
    }
    else
    {
        if(frame->linesize[0] == pdata->video.pitch)
        {
            dbg_warn("Linesizes (yuv420p):\n"
                     "\tlinesize[0]: %d\n"
                     "\tlinesize[1]: %d\n"
                     "\tlinesize[2]: %d\n",
                     frame->linesize[0],
                     frame->linesize[1],
                     frame->linesize[2]);
            
            memcpy(pdata->video.video_frame, frame->data[0], pdata->video.pitch * pdata->video.height);
            memcpy(pdata->video.video_frame_sup1, frame->data[1], pdata->video.pitch_sup1 * (pdata->video.height+1)/2);
            memcpy(pdata->video.video_frame_sup2, frame->data[2], pdata->video.pitch_sup2 * (pdata->video.height+1)/2);
        }
        else
        {
            copy_pixel_buffers(pdata->video.video_frame,
                               pdata->video.pitch,
                               frame->data[0],
                               frame->linesize[0],
                               frame->height);
            
            copy_pixel_buffers(pdata->video.video_frame_sup1,
                               pdata->video.pitch_sup1,
                               frame->data[1],
                               frame->linesize[1],
                               (frame->height+1)/2);
            
            copy_pixel_buffers(pdata->video.video_frame_sup2,
                               pdata->video.pitch_sup2,
                               frame->data[2],
                               frame->linesize[2],
                               (frame->height+1)/2);
        }
    }
    
    pdata->video.type = VIDEO_YUV;
    pdata->video.pts = frame->pts * av_q2d(pdata->decoder.video_time_base); 
    
    return 0;
}


#define copy_bytes(dst, src, bytes)\
do {\
    for(int i = 0; i < bytes; i++)\
    {\
        *((dst)+i) = *((src)+i);\
    }\
} while(0)

int32
process_audio_frame(program_data *pdata, struct frame_info info)
{
    if(pdata->audio.is_ready)
    {
        dbg_warn("Started processing an audio frame, while the previous one hasn't been used.\n");
        return -1;
    }
    
    AVFrame *frame = info.frame;
    decoder_info *decoder = &pdata->decoder;
    
    int32 size = av_samples_get_buffer_size(NULL,
                                            //decoder->audio_codec_context->channels,
                                            frame->channels,
                                            frame->nb_samples,
                                            //decoder->audio_codec_context->sample_fmt,
                                            frame->format,
                                            1);
    
    uint32 SampleCount = frame->nb_samples;
    uint32 Frequency = decoder->audio_codec_context->sample_rate;
    uint32 Channels = decoder->audio_codec_context->channels;
    
    uint32 sample_fmt = decoder->audio_codec_context->sample_fmt; 
    bool32 is_planar = av_sample_fmt_is_planar(sample_fmt);
    uint32 bytes_per_sample = av_get_bytes_per_sample(sample_fmt);//frame->linesize[0]/frame->nb_samples;
    
    uint32 real_size = size; // frame->linesize[0] * decoder->audio_codec_context->channels;
    
    void *data = pdata->audio.buffer;
    if(!data)
    {
        dbg_error("Audio buffer wasn't allocated. Returning.\n");
        return -1;
    }
    
    if(!is_planar)
    {
        memcpy(data + pdata->audio.size, frame->data[0], real_size);
    }
    else
    {
        // NOTE(Val): manually interleaving audio for however
        // many channels
        uint8* dst = data + pdata->audio.size;
        
        for(int c = 0; c < Channels; c++)
        {
            for(int s = 0; s < SampleCount; s++)
            {
                copy_bytes(dst + (s*Channels*bytes_per_sample) + c*bytes_per_sample,
                           frame->data[c] + s*bytes_per_sample,
                           bytes_per_sample);
            }
        }
    }
    
    pdata->audio.buffer = data;
    pdata->audio.size += real_size;
    pdata->audio.duration += (real64)SampleCount / (real64)Frequency;
    //dbg_info("Audio frame duration: %lf\n", pdata->audio.duration);
    
    //pdata->audio.is_ready = 1;
    
    return 0;
}

void
SortPackets(program_data *pdata)
{
    AVPacket *pkt;
    // NOTE(Val): if there are packets in main queue and audio/video queues aren't full
    while(!pq_is_empty(pdata->pq_main) &&
          !pq_is_full(pdata->pq_audio) &&
          !pq_is_full(pdata->pq_video))
    {
        dequeue_packet(pdata->pq_main, &pkt);
        
        if(pkt->stream_index == pdata->decoder.video_stream)
        {
            //dbg_info("Queued video packet.\n");
            enqueue_packet(pdata->pq_video, pkt);
        }
        else if(pkt->stream_index == pdata->decoder.audio_stream)
        {
            //dbg_info("Queued audio packet.\n");
            enqueue_packet(pdata->pq_audio, pkt);
        }
        else
        {
            dbg_info("Discarded unknown packet.\n");
        }
        
        pkt = NULL;
    }
}

void
LoadPackets(program_data *pdata)
{
    while(!pq_is_full(pdata->pq_main) &&
          !pdata->file.file_finished &&
          pdata->running)
    {
        AVPacket *pkt = av_packet_alloc();
        
        int ret = get_packet(pdata, pkt);
        
        if(ret >= 0)
        {
            enqueue_packet(pdata->pq_main, pkt);
        }
        else
        {
            dbg_error("get_packet() failed.\n");
            break;
        }
    }
    
    //if(pq_is_empty(pdata->pq_main))
    //pdata->file.file_finished = 1;
}

int32 
DecodingThreadStart(void *ptr)
{
    program_data *pdata = ptr;
    open_file_info *file = &pdata->file;
    decoder_info *decoder = &pdata->decoder;
    playback_data *playback = &pdata->playback;
    
    decoder->condition = PlatformCreateConditionVar();
    
    while(!pq_is_full(pdata->pq_audio) &&
          !pq_is_full(pdata->pq_video) &&
          !file->file_finished)
    {
        LoadPackets(pdata);
        SortPackets(pdata);
    }
    
    bool32 start_notified = 0;
    
    //ProfilerStart("decoder.prof.log");
    while(pdata->running && !pdata->playback_finished)
    {
        //dbg_success("Processing loop start.\n");
        if(!pdata->audio.is_ready)
        {
            // TODO(Val): Check this loop
            dbg_success("Audio not marked ready.\n");
            
            if(!pq_is_empty(pdata->pq_audio))
            {
                do {
                    dbg_success("Audio packets not empty, starting to process.\n");
                    
                    struct frame_info f = get_frame(pdata, pdata->pq_audio);
                    
                    if(f.ret != -1)
                    {
                        dbg_success("Processing audio.\n");
                        
                        process_audio_frame(pdata, f);
                        av_frame_free(&f.frame);
                    }
                    else
                    {
                        dbg_error("Audio queue was empty.\n");
                    }
                    
                    
                    /*
                    dbg_info("total queued:\t%lf\n"
                    "pdata->audio.duration:\t%lf\n"
                    "sum:\t\t\t%lf\n"
                    "playback start: %lf\n"
                    "next frame time:\t%lf\n"
                    "following frame time:\t%lf\n",
                    pdata->playback.audio_total_queued,
                    pdata->audio.duration,
                    pdata->audio.pts + pdata->audio.duration,
                    pdata->playback.playback_start,
                    pdata->playback.next_frame_time - pdata->playback.playback_start,
                    pdata->playback.next_frame_time + pdata->client.refresh_rate - pdata->playback.playback_start);
                    */
                    
                    if(!pdata->running)
                        break;
                    // TODO(Val): This will only function while we don't miss frames
                } while(playback->audio_total_queued + pdata->audio.duration < 
                        get_next_playback_time(playback));
                //} while(!pdata->file.file_finished &&
                //(pdata->playback.audio_total_queued + pdata->audio.duration) < (pdata->playback.next_frame_time + pdata->client.refresh_target - pdata->playback.playback_start));
                
                pdata->audio.is_ready = 1;
            }
            else
            {
                dbg_error("There were no audio packets.\n");
            }
        }
        
        if(!pdata->video.is_ready && !pq_is_empty(pdata->pq_video))
        {
            struct frame_info f = get_frame(pdata, pdata->pq_video);
            
            if(f.ret != -1)
            {
                dbg_success("Processing video.\n");
                
                int32 ret = process_video_frame(pdata, f);
                if(ret < 0)
                {
                    dbg_error("process_video_frame() failed.\n");
                }
                
                av_frame_free(&f.frame);
            }
            else
            {
                dbg_error("get_frame failed.\n");
            }
            
            pdata->video.is_ready = 1;
        }
        else if(!pdata->video.is_ready)
        {
            dbg_error("No video packets to process.\n");
        }
        else
        {
            
        }
        
        // TODO(Val): This may not be fool proof
        //if(pq_is_empty(pdata->pq_video) && pq_is_empty(pdata->pq_audio))
        //pdata->playback_finished = 1;
        //else
        //{
        
        if(!start_notified && pdata->audio.is_ready && pdata->video.is_ready)
        {
            start_notified = 1;
            pdata->start_playback = 1;
        }
        //dbg_info("Decoder: Starting condition wait.\n");
        
        while(!pq_is_full(pdata->pq_audio) &&
              !pq_is_full(pdata->pq_video) &&
              !file->file_finished)
        {
            LoadPackets(pdata);
            SortPackets(pdata);
        }
        
        PlatformConditionWait(&decoder->condition);
        //dbg_success("Decoder: Thread woken up.\n");
        //}
    }
    
    PlatformConditionDestroy(&decoder->condition);
    
    return 0;
}
