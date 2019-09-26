#include <libswscale/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

//#include "defines.h"
#include "watchtogether.h"
#include "timing.h"
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
        RETURN(WRONG_ARGS);
    }
    else if(ret == AVERROR_EOF)
    {
        // TODO(Val): Mark that there are no more packets for this file. 
        pdata->file.file_finished = 1;
        
        dbg_error("End of file.\n");
        RETURN(FILE_EOF);
    }
    else if(ret != 0)
    {
        dbg_error("Some other error happened.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

int32
get_frame(program_data *pdata, AVFrame **frame, avpacket_queue *queue, AVCodecContext *dec_ctx)
{
    StartTimer("get_frame()");
    
    int ret = 0;
    
    decoder_info *decoder = &pdata->decoder;
    AVPacket *pkt = NULL;// = av_packet_alloc();
    
    *frame = av_frame_alloc();
    if(!frame)
        RETURN(NO_MEMORY);
    
    do {
        if(f(peek_packet(queue, &pkt, 0)))
        {
            dbg_error("There were no packets in queue.\n");
            av_frame_free(frame);
            RETURN(NOT_ENOUGH_DATA);
        }
        
        ret = avcodec_send_packet(dec_ctx, pkt);
        
        if(ret == 0)
        {
            av_packet_unref(pkt);
            dequeue_packet(queue, &pkt);
        }
        else if(ret == AVERROR(EAGAIN))
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
            RETURN(WRONG_ARGS);
            // NOTE(Val): codec not opened, it is an encoder, or requires flush
        }
        else if(ret == AVERROR(ENOMEM))
        {
            dbg_error("AVERROR(ENOMEM)\n");
            RETURN(NO_MEMORY);
            // NOTE(Val): No memory
        }
        else if(ret < 0)
        {
            dbg_error("Decoding error.\n");
        }
        
        ret = avcodec_receive_frame(dec_ctx, *frame);
        //dbg_info("avcodec_receive_frame\tret: %d\n", ret);
        
        if(ret == AVERROR(EAGAIN))
        {
            continue;
            // NOTE(Val): This means a frame must be read before we can send another packet
        }
        else if(ret == AVERROR_EOF)
        {
            // TODO(Val): Mark file as finished.
            dbg_warn("AVERROR_EOF\n");
            RETURN(FILE_EOF);
        }
        else if(ret == AVERROR(EINVAL))
        {
            dbg_warn("AVERROR(EINVAL)\n");
            // NOTE(Val): codec not opened, it is an encoder, or requires flush
            RETURN(WRONG_ARGS);
        }
        else if(ret < 0)
        {
            dbg_error("Decoding error.\n");
        }
    } while(ret == AVERROR(EAGAIN) && pdata->running);
    
    EndTimer();
    RETURN(SUCCESS);
}

int32
DecodingFileOpen(open_file_info *file, decoder_info *decoder)
{
    // Open video file
    dbg_print("avformat version: %d - %d\n", LIBAVFORMAT_VERSION_INT, avformat_version());
    
    if(avformat_open_input(&decoder->format_context, file->filename, NULL, NULL) < 0)
    {
        dbg_error("AV open input failed.\n");
        RETURN(FILE_NOT_FOUND); // Couldn't open file
    }
    
    // Retrieve stream information
    if(avformat_find_stream_info(decoder->format_context, NULL) < 0)
    {
        dbg_error("Couldn't find stream info\n");
        RETURN(UNKNOWN_ERROR); // Couldn't find stream information
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
        dbg_warn("AVERROR_STREAM_NOT_FOUND\n");
        file->has_audio = 0;
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
            RETURN(UNKNOWN_ERROR);
        }
        avcodec_parameters_to_context(decoder->video_codec_context, decoder->format_context->streams[decoder->video_stream]->codecpar);
        
        if(avcodec_open2(decoder->video_codec_context, decoder->video_codec, NULL) < 0)
        {
            dbg_error("Video: avcodec_open2() failed!\n");
            RETURN(UNKNOWN_ERROR);
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
        file->fps = av_q2d(time);
        file->target_time = av_q2d(av_inv_q(time));
        
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
            RETURN(UNKNOWN_ERROR);
        }
        avcodec_parameters_to_context(decoder->audio_codec_context, decoder->format_context->streams[decoder->audio_stream]->codecpar);
        
        if(avcodec_open2(decoder->audio_codec_context, decoder->audio_codec, NULL) < 0)
        {
            dbg_error("Audio: avcodec_open2() failed!\n");
            RETURN(UNKNOWN_ERROR);
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
            RETURN(UNKNOWN_FORMAT);
        }
        
        file->sample_rate = decoder->audio_codec_context->sample_rate;
        file->bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
        file->channels = decoder->audio_codec_context->channels;
        file->sample_format = fmt;
        
        file->has_audio = 1;
        
        decoder->audio_time_base = av_inv_q(decoder->format_context->streams[decoder->audio_stream]->avg_frame_rate);
    }
    
    RETURN(SUCCESS);
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
process_video_frame(program_data *pdata, AVFrame *frame)
{
    StartTimer("process_video_frame");
    
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
        
        StartTimer("sws_scale()");
        sws_scale(modifContext,
                  (uint8 const* const*)frame->data,
                  frame->linesize,
                  0,
                  decoder->video_codec_context->height,
                  (uint8* const* const)ptrs,
                  stride);
        EndTimer();
        
        EndTimer();
    }
    else
    {
        StartTimer("Copying YUV Frame");
        
        if(frame->linesize[0] == pdata->video.pitch)
        {
            dbg_warn("Linesizes (yuv420p):\n"
                     "\tlinesize[0]: %d\n"
                     "\tlinesize[1]: %d\n"
                     "\tlinesize[2]: %d\n",
                     frame->linesize[0],
                     frame->linesize[1],
                     frame->linesize[2]);
            
            StartTimer("memcpy x3");
            memcpy(pdata->video.video_frame, frame->data[0], pdata->video.pitch * pdata->video.height);
            memcpy(pdata->video.video_frame_sup1, frame->data[1], pdata->video.pitch_sup1 * (pdata->video.height+1)/2);
            memcpy(pdata->video.video_frame_sup2, frame->data[2], pdata->video.pitch_sup2 * (pdata->video.height+1)/2);
            EndTimer();
        }
        else
        {
            StartTimer("copy_pixel_buffers()");
            
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
            
            EndTimer();
        }
        
        EndTimer();
    }
    
    pdata->video.type = VIDEO_YUV;
    pdata->video.pts = frame->pts * av_q2d(pdata->decoder.video_time_base); 
    
    EndTimer();
    
    RETURN(SUCCESS);
}


#define copy_bytes(dst, src, bytes)\
do {\
    for(int i = 0; i < bytes; i++)\
    {\
        *((dst)+i) = *((src)+i);\
    }\
} while(0)

int32
process_audio_frame(program_data *pdata, AVFrame *frame)
{
    StartTimer("process_audio_frame");
    
    if(pdata->audio.is_ready)
    {
        dbg_warn("Started processing an audio frame, while the previous one hasn't been used.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
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
        
        EndTimer();
        RETURN(NOT_INITIALIZED);
    }
    
    if(!is_planar)
    {
        StartTimer("memcpy");
        memcpy(data + pdata->audio.size, frame->data[0], real_size);
        EndTimer();
    }
    else
    {
        // NOTE(Val): manually interleaving audio for however
        // many channels
        StartTimer("Interleaving Audio");
        
        uint8* dst = data + pdata->audio.size;
        dbg_print("Audio size: %d/%d\n", pdata->audio.size, pdata->audio.max_buffer_size);
        
        for(int s = 0; s < SampleCount; s++)
        {
            for(int c = 0; c < Channels; c++)
            {
                for(int b = 0; b < bytes_per_sample; b++)
                {
                    // TODO(Val): Check if this is correct
                    *(dst + s*bytes_per_sample*Channels + bytes_per_sample*c + b) = *(frame->data[c] + s*bytes_per_sample + b);
                }
            }
        }
        
        EndTimer();
    }
    
    pdata->audio.size += real_size;
    dbg_print("audio duration: %lf\n", pdata->audio.duration);
    pdata->audio.duration += (real64)SampleCount / (real64)Frequency;
    dbg_print("new audio duration: %lf\n", pdata->audio.duration);
    //dbg_info("Audio frame duration: %lf\n", pdata->audio.duration);
    
    EndTimer();
    RETURN(SUCCESS);
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
    
    InitializeTimingSystem();
    
    StartTimer("Decoding Start");
    
    StartTimer("Fill Packets");
    while(!pq_is_full(pdata->pq_audio) &&
          !pq_is_full(pdata->pq_video) &&
          !file->file_finished)
    {
        LoadPackets(pdata);
        SortPackets(pdata);
    }
    EndTimer();
    
    bool32 start_notified = 0;
    
    //ProfilerStart("decoder.prof.log");
    StartTimer("Decoding Loop");
    while(pdata->running && !pdata->playback_finished)
    {
        StartTimer("Decoding Loop Pass");//dbg_success("Processing loop start.\n");
        
        StartTimer("Decoding Loop Processing");
        
        if(pdata->file.has_audio)
        {
            if(!pdata->audio.is_ready && !pq_is_empty(pdata->pq_audio))
            {
                StartTimer("Processing Audio");
                
                dbg_print("Processing Audio:\n"
                          "\tready: %d\n"
                          "\tplayback->audio_total_queued: %lf\n"
                          "\tplayback->audio_total_decoded: %lf\n",
                          pdata->audio.is_ready,
                          playback->audio_total_queued,
                          playback->audio_total_decoded);
                
                if(!pdata->audio.is_ready && 
                   playback->audio_total_queued == playback->audio_total_decoded)
                {
                    PlatformLockMutex(&pdata->audio.mutex);
                    
                    do {
                        dbg_success("Audio packets not empty, starting to process. %lf\n", pdata->file.target_time);
                        
                        AVFrame *frame;
                        if(!get_frame(pdata, &frame, pdata->pq_audio, pdata->decoder.audio_codec_context))
                        {
                            dbg_success("Processing audio.\n");
                            
                            process_audio_frame(pdata, frame);
                            av_frame_free(&frame);
                        }
                        else
                        {
                            dbg_error("Audio queue was empty.\n");
                        }
                        
                        if(!pdata->running)
                            break;
                    } while (pdata->audio.duration <= pdata->file.target_time);
                    
                    dbg_error("Setting audio readiness.\n");
                    pdata->playback.audio_total_decoded += pdata->audio.duration;
                    pdata->audio.is_ready = 1;
                    
                    PlatformUnlockMutex(&pdata->audio.mutex);
                }
                else if(pdata->audio.is_ready)
                {
                    dbg_error("Decoding: Audio was already ready.\n");
                }
                else
                {
                    dbg_error("Decoding: Queued audio and decoded audio not equal.\n");
                }
                
                EndTimer();
            }
            else
            {
                dbg_success("Audio not marked ready or no audio packets left.\n");
            }
        }
        
        if(pdata->file.has_video)
        {
            if(!pdata->video.is_ready && !pq_is_empty(pdata->pq_video))
            {
                StartTimer("Processing Video");
                
                AVFrame *frame;
                if(!get_frame(pdata, &frame, pdata->pq_video, pdata->decoder.video_codec_context))
                {
                    dbg_success("Processing video.\n");
                    
                    int32 ret = process_video_frame(pdata, frame);
                    if(ret < 0)
                    {
                        dbg_error("process_video_frame() failed.\n");
                    }
                    
                    StartTimer("av_frame_free()");
                    av_frame_free(&frame);
                    EndTimer();
                }
                else
                {
                    dbg_error("get_frame failed.\n");
                }
                
                pdata->video.is_ready = 1;
                
                EndTimer();
            }
            else if(!pdata->video.is_ready)
            {
                dbg_error("No video packets to process.\n");
            }
            else
            {
                //dbg_info("Video frame has not been used yet.\n");
            }
        }
        // TODO(Val): This may not be fool proof
        //if(pq_is_empty(pdata->pq_video) && pq_is_empty(pdata->pq_audio))
        //pdata->playback_finished = 1;
        //else
        //{
        
        if(!start_notified &&
           (!pdata->file.has_audio || pdata->audio.is_ready) &&
           (!pdata->file.has_video || pdata->video.is_ready))
        {
            start_notified = 1;
            pdata->start_playback = 1;
        }
        //dbg_info("Decoder: Starting condition wait.\n");
        
        dbg_info("Running packet loading.\n");
        if(pq_is_full(pdata->pq_audio))
            dbg_error("Audio queue full.\n");
        if(pq_is_full(pdata->pq_video))
            dbg_error("Video queue full.\n");
        if(file->file_finished)
            dbg_error("File marked finished.\n");
        
        StartTimer("Queue Packets");
        while(!pq_is_full(pdata->pq_audio) &&
              !pq_is_full(pdata->pq_video) &&
              !file->file_finished)
        {
            LoadPackets(pdata);
            SortPackets(pdata);
        }
        EndTimer();
        
        EndTimer();
        
        StartTimer("Condition Wait Start");
        PlatformConditionWait(&decoder->condition);
        EndTimer();
        
        EndTimer();
        //dbg_success("Decoder: Thread woken up.\n");
        //}
    }
    EndTimer();
    
    EndTimer();
    
    FinishTiming();
    
    return 0;
}
