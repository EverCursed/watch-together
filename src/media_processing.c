#include "media_processing.h"

#include <libswscale/swscale.h>


#include "defines.h"
#include "watchtogether.h"
#include "decoding.h"
#include "encoding.h"
#include "file_data.h"
#include "utils/timing.h"
#include "packet_queue.h"
#include "time.h"
#include "debug.h"
#include "streaming.h"

static inline int32 is_video(AVPacket *packet, decoder_info *decoder) {return (packet->stream_index == decoder->video_stream->index);}
static inline int32 is_audio(AVPacket *packet, decoder_info *decoder) {return (packet->stream_index == decoder->audio_stream->index);}
static inline int32 is_subtitle(AVPacket *packet, decoder_info *decoder) {return (packet->stream_index == decoder->subtitle_stream->index);}

static int32
LoadPacket(decoder_info *decoder, AVPacket **packet)
{
    *packet = av_packet_alloc();
    int ret = av_read_frame(decoder->format_context, *packet);
    
    if(ret == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("Stream not found.\n");
        RETURN(WRONG_ARGS);
    }
    else if(ret == AVERROR_EOF)
    {
        // TODO(Val): Mark that there are no more packets for this file. 
        dbg_error("End of file.\n");
        decoder->file_fully_loaded = 1;
        RETURN(FILE_EOF);
    }
    else if(ret != 0)
    {
        dbg_error("Some other error happened.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

static int32
process_audio_frame(AVFrame *frame, output_audio *audio, decoder_info *decoder)
{
    StartTimer("process_audio_frame");
#if 0
    if(audio->is_ready)
    {
        dbg_warn("Started processing an audio frame, while the previous one hasn't been used.\n");
        EndTimer();
        RETURN(UNKNOWN_ERROR);
    }
#endif
    int32 size = av_samples_get_buffer_size(NULL,
                                            frame->channels,
                                            frame->nb_samples,
                                            frame->format,
                                            1);
    
    uint32 SampleCount = frame->nb_samples;
    uint32 Frequency = decoder->audio_codec_context->sample_rate;
    uint32 Channels = decoder->audio_codec_context->channels;
    
    uint32 sample_fmt = decoder->audio_codec_context->sample_fmt; 
    uint32 bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
    bool32 is_planar = av_sample_fmt_is_planar(sample_fmt);
    
    uint32 real_size = size;
    
    dbg_print("Audio processing:\n"
              "\tSampleCount:\t%d\n"
              "\tFrequency:\t%d\n"
              "\tChannels:\t%d\n",
              SampleCount,
              Frequency,
              Channels);
    
    void *data = audio->buffer;
    if(!data)
    {
        dbg_error("Audio buffer wasn't allocated. Returning.\n");
        
        EndTimer();
        RETURN(NOT_INITIALIZED);
    }
    
    PlatformLockMutex(&audio->mutex);
    if(!is_planar)
    {
        StartTimer("memcpy");
        memcpy(data + audio->size, frame->data[0], real_size);
        EndTimer();
    }
    else
    {
        // NOTE(Val): manually interleaving audio for however
        // many channels
        StartTimer("Interleaving Audio");
        
        uint8* dst = data + audio->size;
        dbg_print("Audio size: %d/%d\n", audio->size, audio->max_buffer_size);
        
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
    PlatformUnlockMutex(&audio->mutex);
    
    audio->size += real_size;
    dbg_print("audio duration: %lf\n", audio->duration);
    
    real64 duration = (real64)SampleCount / (real64)Frequency;
    audio->duration += duration;
    dbg_print("new audio duration: %lf\n", audio->duration);
    //dbg_info("Audio frame duration: %lf\n", pdata->audio.duration);
    
    audio->is_ready = 1;
    EndTimer();
    RETURN(SUCCESS);
}

global struct SwsContext* modifContext = NULL;

int32
process_video_frame(AVFrame *frame, output_video *video, decoder_info *decoder)
{
    StartTimer("process_video_frame");
    
    int32 fmt = AV_PIX_FMT_RGB32;
    
    //if(frame->format != AV_PIX_FMT_RGB24)
    //{
    modifContext = sws_getCachedContext(modifContext,
                                        frame->width,
                                        frame->height,
                                        frame->format,
                                        video->width,  // dst width
                                        video->height, // dst height
                                        fmt,
                                        SWS_BILINEAR, //SWS_BILINEAR | SWS_ACCURATE_RND,
                                        NULL, NULL, NULL);
    
    uint8_t *ptrs[1] = {
        video->video_frame,//frame_Y,
        //pdata->video.video_frame_sup1,//frame_U,
        //pdata->video.video_frame_sup2,//frame_V
    };
    
    int stride[1] = {
        video->pitch, //pitch_Y,
        //pdata->video.pitch_sup1, //pitch_U,
        //pdata->video.pitch_sup2, //pitch_V
    };
    
    StartTimer("sws_scale()");
    sws_scale(modifContext,
              (uint8 const* const*)frame->data,
              frame->linesize,
              0,
              video->height,
              (uint8* const* const)ptrs,
              stride);
    EndTimer();
    
    video->type = VIDEO_RGB;
    video->pts = frame->pts * av_q2d(decoder->video_time_base); 
    video->frame_duration = decoder->avg_video_framerate;
    
    video->is_ready = 1;
    EndTimer();
    
    RETURN(SUCCESS);
}

#define VIDEO    1
#define AUDIO    2
#define SUBTITLE 3

static int32
ProcessPacket(AVFrame **frame, int32 *type, AVPacket *packet, decoder_info *decoder)
{
    StartTimer("ProcessPacket()");
    int ret = 0;
    
    AVCodecContext *codec_context = NULL;
    if(is_video(packet, decoder))
    {
        codec_context = decoder->video_codec_context;
        *type = VIDEO;
    }
    else if(is_audio(packet, decoder))
    {
        codec_context = decoder->audio_codec_context;
        *type = AUDIO;
    }
    else if(is_subtitle(packet, decoder))
    {
        // TODO(Val): handle this
        // *type = SUBTITLE;
    }
    else
    {
        dbg_error("Unknown type of packet.\n");
    }
    
    ret = DecodePacket(frame, packet, codec_context);
    if(ret == NEED_DATA)
        RETURN(NEED_DATA);
    
    EndTimer();
    RETURN(SUCCESS);
}
/*
if(!audio->is_ready && !pq_is_empty(pdata->pq_audio))
{
    StartTimer("Processing Audio");
    
    dbg_print("Processing Audio:\n"
              "\tready: %d\n"
              "\tplayback->audio_total_queued: %lf\n"
              "\tplayback->audio_total_decoded: %lf\n",
              audio->is_ready,
              playback->audio_total_queued,
              playback->audio_total_decoded);
              
    if(!audio->is_ready && 
       playback->audio_total_queued + pdata->client.refresh_target >= playback->audio_total_decoded)
    {
        PlatformLockMutex(&audio->mutex);
        
        AVFrame *frame;
        frame = DecodePacket(pdata->pq_audio, pdata->decoder.audio_codec_context);
        
        do {
            dbg_success("Audio packets not empty, starting to process. %lf\n", file->target_time);
            
            if(frame)
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
        } while (audio->duration <= file->target_time);
        
        dbg_error("Setting audio readiness.\n");
        pdata->playback.audio_total_decoded += audio->duration;
        audio->is_ready = 1;
        
        PlatformUnlockMutex(&audio->mutex);
    }
    else if(audio->is_ready)
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

if(file->has_video)
{
    if(!pdata->video.is_ready && !pq_is_empty(pdata->pq_video))
    {
        StartTimer("Processing Video");
        
        AVFrame *frame;
        if((frame = DecodePacket(pdata->pq_video, pdata->decoder.video_codec_context)))
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
//dbg_info("Decoder: Starting condition wait.\n");

EndTimer();
}
*/
/*
static void
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
*/
/*
static void
EnqueuePackets(program_data *pdata)
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
}
static void
LoadPackets(program_data *pdata, open_file_info *file)
{
    StartTimer("Load Packets");
    
    if(pdata->is_partner)
    {
        // TODO(Val): test if this works with streamed files.
        while(!pq_is_full(pdata->pq_audio) &&
              !pq_is_full(pdata->pq_video) &&
              !file->file_finished)
        {
            EnqueuePackets(pdata);
            SortPackets(pdata);
        }
    }
    else
    {
        while(!pq_is_full(pdata->pq_audio) &&
              !pq_is_full(pdata->pq_video) &&
              !file->file_finished)
        {
            EnqueuePackets(pdata);
            SortPackets(pdata);
        }
    }
    
    EndTimer();
}
*/

#define PACKET_BUFFER 30

int32
MediaOpen(open_file_info *file, decoder_info *decoder, encoder_info *encoder)
{
    // Open video file
    dbg_print("avformat version: %d - %d\n", LIBAVFORMAT_VERSION_INT, avformat_version());
    
    decoder->queue = init_avpacket_queue(PACKET_BUFFER);
    
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
    decoder->video_stream_index = av_find_best_stream(decoder->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder->video_codec, 0);
    decoder->audio_stream_index = av_find_best_stream(decoder->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder->audio_codec, 0);
    decoder->subtitle_stream_index = av_find_best_stream(decoder->format_context, AVMEDIA_TYPE_SUBTITLE, -1, -1, &decoder->subtitle_codec, 0);
    
    if(decoder->video_stream_index >= 0)
    {
        dbg_success("av_find_best_steam successful for video.\n");
    }
    else if(decoder->video_stream_index == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_error("AVERROR_STREAM_NOT_FOUND\n");
        file->has_video = 0;
    }
    else if(decoder->video_stream_index == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    if(decoder->audio_stream_index >= 0)
    {
        dbg_success("av_find_best_steam successful for audio.\n");
    }
    else if(decoder->audio_stream_index == AVERROR_STREAM_NOT_FOUND)
    {
        dbg_warn("AVERROR_STREAM_NOT_FOUND\n");
        file->has_audio = 0;
    }
    else if(decoder->audio_stream_index == AVERROR_DECODER_NOT_FOUND)
    {
        dbg_error("AVERROR_DECODER_NOT_FOUND\n");
    }
    
    // Init video codec context
    if(decoder->video_stream_index >= 0)
    {
        decoder->video_codec_context = avcodec_alloc_context3(decoder->video_codec);
        if(!decoder->video_codec_context)
        {
            dbg_error("Video: avcodec_alloc_context3() failed!\n");
            RETURN(UNKNOWN_ERROR);
        }
        avcodec_parameters_to_context(decoder->video_codec_context, decoder->format_context->streams[decoder->video_stream_index]->codecpar);
        
        if(avcodec_open2(decoder->video_codec_context, decoder->video_codec, NULL) < 0)
        {
            dbg_error("Video: avcodec_open2() failed!\n");
            RETURN(UNKNOWN_ERROR);
        }
        
        if(decoder->format_context->streams[decoder->video_stream_index]->codecpar->format == AV_PIX_FMT_YUV420P)
        {
            file->video_format = VIDEO_YUV;
        }
        else //if(format_context->streams[video_stream]->codecpar->format == AV_PIX_FMT_RGB32)
        {
            file->video_format = VIDEO_YUV;
        }
        
        file->width = decoder->video_codec_context->width;
        file->height = decoder->video_codec_context->height;
        
        AVRational time = decoder->format_context->streams[decoder->video_stream_index]->avg_frame_rate;
        file->fps = av_q2d(time);
        file->target_time = av_q2d(av_inv_q(time));
        
        file->has_video = 1;
        
        decoder->video_time_base = //av_inv_q(
            decoder->format_context->streams[decoder->video_stream_index]->time_base;
        decoder->avg_video_framerate = file->target_time;
        
        decoder->video_stream = decoder->format_context->streams[decoder->video_stream_index];
    }
    
    if(decoder->audio_stream_index >= 0)
    {
        // Init audio codec context
        decoder->audio_codec_context = avcodec_alloc_context3(decoder->audio_codec);
        if(!decoder->audio_codec_context)
        {
            dbg_error("Audio: avcodec_alloc_context3() failed!\n");
            RETURN(UNKNOWN_ERROR);
        }
        avcodec_parameters_to_context(decoder->audio_codec_context, decoder->format_context->streams[decoder->audio_stream_index]->codecpar);
        
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
        
        decoder->audio_time_base = av_inv_q(decoder->format_context->streams[decoder->audio_stream_index]->avg_frame_rate);
        decoder->audio_stream = decoder->format_context->streams[decoder->audio_stream_index];
    }
    
    RETURN(SUCCESS);
}

int32
MediaClose(open_file_info *file, decoder_info *decoder, encoder_info *encoder)
{
    // TODO(Val): Should we also stop running the current file?
    
    avformat_free_context(decoder->format_context);
    
    avcodec_free_context(&decoder->audio_codec_context);
    avcodec_free_context(&decoder->video_codec_context);
    
    decoder->audio_codec = NULL;
    decoder->video_codec = NULL;
    
    close_avpacket_queue(decoder->queue);
    
    RETURN(SUCCESS);
}

static void
ProcessEverything(decoder_info *decoder, output_video *video, output_audio *audio)
{
    StartTimer("ProcessEverything()");
    
    int ret = 0;
    
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet;
    
    load_another_packet:
    ret = dequeue_packet(decoder->queue, &packet);
    if(ret == FILE_EOF)
    {
        // TODO(Val): Mark file as fully read
    }
    else if(fail(ret))
    {
        return;
        // TODO(Val): make this more foolproof
        //dbg_print
    }
    
    int32 media_type = 0;
    ret = ProcessPacket(&frame, &media_type, packet, decoder);
    if(ret == NEED_DATA)
    {
        av_packet_unref(packet);
        goto load_another_packet;
    }
    
    if(media_type == VIDEO)
    {
        //if(!video->is_ready)
        process_video_frame(frame, video, decoder);
    }
    else if(media_type == AUDIO)
    {
        //if(!audio->is_ready)
        process_audio_frame(frame, audio, decoder);
    }
    else if(media_type == SUBTITLE)
    {
        // TODO(Val): handle this
    }
    else 
    {
        // TODO(Val): unknown type
    }
    
    av_packet_unref(packet);
    av_frame_unref(frame);
    
    EndTimer();
}

static bool32
EnoughDurations(output_audio *audio, open_file_info *file, output_video *video, playback_data *playback)
{
    StartTimer("EnoughDurations()");
    
    real64 audio_time = audio->duration;
    // TODO(Val): This will break the application if the framerate is larger than the monitor refresh rate.
    real64 video_time = video->frame_duration;
    real64 refresh_time = *playback->refresh_target;
    real64 next_time = get_future_playback_time(playback);
    
    bool32 audio_enough =
        !file->has_audio ||
        (audio->is_ready && ((playback->audio_total_queued + audio->duration) >= next_time)) ||
        (playback->audio_total_queued >= next_time);
    bool32 video_enough = !file->has_video || (video->is_ready && ((video->pts + video->frame_duration) >= next_time));
    
    dbg_print("EnoughDurations():\n"
              "\taudio_enough: %s\taudio time: %lf\n"
              "\tvideo_enough: %s\tvideo time: %lf\n"
              "\tnext_time: %lf\n",
              b2str(audio_enough), (audio->duration+playback->audio_total_queued),
              b2str(video_enough), (video->pts + video->frame_duration),
              next_time);
    
    //return (refresh_target < smallest(audio_time, video_time)) ;
    EndTimer();
    
    return audio_enough && video_enough;
}

static int32
RefillPackets(decoder_info *decoder, bool32 is_host)
{
    while(!pq_is_full(decoder->queue) && !decoder->file_fully_loaded)
    {
        AVPacket *packet;
        LoadPacket(decoder, &packet);
        
        enqueue_packet(decoder->queue, packet);
        
        if(is_host)
            Streaming_Host_SendPacket(decoder, packet);
    }
    
    RETURN(SUCCESS);
}

int32
MediaThreadStart(void *arg)
{
    InitializeTimingSystem("media");
    
    program_data *pdata = arg;
    open_file_info *file = &pdata->file;
    decoder_info *decoder = &pdata->decoder;
    //encoder_info *encoder = &pdata->encoder;
    playback_data *playback = &pdata->playback;
    
    if(pdata->is_host)
        Streaming_Host_Initialize(decoder);
    
    int ret = 0;
    
    RefillPackets(decoder, pdata->is_host);
    
    bool32 start_notified = 0;
    
    StartTimer("Processing Loop");
    while(pdata->running && !pdata->playback_finished)
    {
        StartTimer("Start processing loop");
        while(!EnoughDurations(&pdata->audio, &pdata->file, &pdata->video, playback))
        {
            ProcessEverything(decoder, &pdata->video, &pdata->audio);
            RefillPackets(decoder, pdata->is_host);
        }
        EndTimer();
        
        dbg_print("Playback start check:\n"
                  "\tstart_notified:  %s\n"
                  "\tfile.has_audio: %s\n"
                  "\taudio.is_ready:  %s\n"
                  "\tfile.has_video: %s\n"
                  "\tvideo.is_ready:  %s\n",
                  b2str(start_notified),
                  b2str(pdata->file.has_audio),
                  b2str(pdata->audio.is_ready),
                  b2str(pdata->file.has_video),
                  b2str(pdata->video.is_ready));
        
        
        if(!start_notified &&
           (!pdata->file.has_audio || pdata->audio.is_ready) &&
           (!pdata->file.has_video || pdata->video.is_ready))
        {
            start_notified = 1;
            pdata->start_playback = 1;
        }
        
        StartTimer("RefillPackets()");
        if(!decoder->file_fully_loaded)
            RefillPackets(decoder, pdata->is_host);
        EndTimer();
        
        StartTimer("Waiting");
        PlatformConditionWait(&decoder->condition);
        EndTimer();
    }
    EndTimer();
    
    if(pdata->is_host)
        Streaming_Host_Close();
    
    
    FinishTiming();
    RETURN(SUCCESS);
}
