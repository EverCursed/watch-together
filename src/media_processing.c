/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

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
#include "avframe_pts_ordered_queue.h"
#include "audio.h"
#include "video.h"

internal inline i32 is_video(AVPacket *packet, decoder_info *decoder)
{ return (packet->stream_index == decoder->video_stream->index); }

internal inline i32 is_audio(AVPacket *packet, decoder_info *decoder)
{ return (packet->stream_index == decoder->audio_stream->index); }

internal inline i32 is_subtitle(AVPacket *packet, decoder_info *decoder)
{ return (packet->stream_index == decoder->subtitle_stream->index); }

internal i32
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

internal void
interleave_audio_frame(AVFrame *dst_frame, AVFrame *src_frame)
{
    u8 *data = dst_frame->data[0];
    i32 channels = src_frame->channels;
    i32 bytes_per_sample = av_get_bytes_per_sample(src_frame->format);
    i32 samples = src_frame->nb_samples;
    
    for(int s = 0; s < samples; s++)
    {
        for(int c = 0; c < channels; c++)
        {
            for(int b = 0; b < bytes_per_sample; b++)
            {
                *(data + s*bytes_per_sample*channels + bytes_per_sample*c + b) = *(src_frame->data[c] + s*bytes_per_sample + b);
            }
        }
    }
}

// TODO(Val): Check for failed allocations
internal i32
process_audio_frame(AVFrame **frame, output_audio *audio, decoder_info *decoder)
{
    StartTimer("process_audio_frame");
    
    AVFrame *new_frame = av_frame_alloc();
    new_frame->nb_samples = (*frame)->nb_samples;
    new_frame->sample_rate = (*frame)->sample_rate;
    new_frame->channels = (*frame)->channels;
    new_frame->format = av_get_packed_sample_fmt((*frame)->format);
    new_frame->pts = (*frame)->pts;
    
    if(!av_frame_get_buffer(new_frame, 0))
    {
        if(av_sample_fmt_is_planar((*frame)->format))
        {
            StartTimer("Interleaving Audio");
            interleave_audio_frame(new_frame, *frame);
            EndTimer();
        }
        else
        {
            memcpy(new_frame->data[0], (*frame)->data[0], new_frame->linesize[0]);
        }
    }
    
    av_frame_free(frame);
    *frame = new_frame;
    
    EndTimer();
    RETURN(SUCCESS);
}

global struct SwsContext* modifContext = NULL;

// TODO(Val): Handle all allocation failures
internal i32
process_video_frame(AVFrame **frame, output_video *video, decoder_info *decoder)
{
    StartTimer("process_video_frame");
    
    AVFrame *new_frame = av_frame_alloc();
    new_frame->format = AV_PIX_FMT_BGRA;
    new_frame->width = (*frame)->width;
    new_frame->height = (*frame)->height;
    new_frame->pts = (*frame)->pts;
    
    if(!av_frame_get_buffer(new_frame, 0))
    {
        modifContext = sws_getCachedContext(modifContext,
                                            (*frame)->width,
                                            (*frame)->height,
                                            (*frame)->format,
                                            new_frame->width,  // dst width
                                            new_frame->height, // dst height
                                            new_frame->format,
                                            SWS_BILINEAR, //SWS_BILINEAR | SWS_ACCURATE_RND,
                                            NULL, NULL, NULL);
        
        /*
                u8_t *ptrs[1] = {
                    video->video_frame,//frame_Y,
                    //pdata->video.video_frame_sup1,//frame_U,
                    //pdata->video.video_frame_sup2,//frame_V
                };
                  
                int stride[1] = {
                    video->pitch, //pitch_Y,
                    //pdata->video.pitch_sup1, //pitch_U,
                    //pdata->video.pitch_sup2, //pitch_V
                };
                */
        StartTimer("sws_scale()");
        sws_scale(modifContext,
                  (u8 const* const*)(*frame)->data,
                  (*frame)->linesize,
                  0,
                  new_frame->height,
                  (u8* const* const)new_frame->data,
                  new_frame->linesize);
        EndTimer();
        
        av_frame_free(frame);
        *frame = new_frame;
        
        EndTimer();
        
        RETURN(SUCCESS);
    }
    
    //video->type = VIDEO_RGB;
    //video->pts = frame->pts * av_q2d(decoder->video_time_base); 
    //video->frame_duration = decoder->avg_video_framerate;
    
    //video->is_ready = 1;
    
    EndTimer();
    RETURN(NO_MEMORY);
}

#define VIDEO    1
#define AUDIO    2
#define SUBTITLE 3

internal i32
ProcessPacket(AVFrame **frame, i32 *type, AVPacket *packet, decoder_info *decoder)
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
    }
    else
    {
        dbg_error("Unknown type of packet.\n");
    }
    
    if(*type == VIDEO || *type == AUDIO)
        ret = DecodePacket(frame, packet, codec_context);
    
    if(ret == NEED_DATA)
        RETURN(NEED_DATA);
    
    EndTimer();
    RETURN(SUCCESS);
}

#define PACKET_BUFFER 30

// TODO(Val): Need to change this so if anything fails during opening, everything is deallocated appropriately
internal i32
MediaOpen(open_file_info *file, decoder_info *decoder, encoder_info *encoder, output_audio *audio, output_video *video)
{
    dbg_print("avformat version: %d - %d\n", LIBAVFORMAT_VERSION_INT, avformat_version());
    
    // initializing all per-media modules
    decoder->queue = PQInit(PACKET_BUFFER);
    if(!decoder->queue) RETURN(NO_MEMORY);
    
    // Open video file
    dbg_info("Opening file: %s\n", file->filename);
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
    
    // video
    {
        if(decoder->video_stream_index >= 0)
        {
            file->has_video = 1;
            
            dbg_success("av_find_best_steam successful for video.\n");
        }
        else if(decoder->video_stream_index == AVERROR_STREAM_NOT_FOUND)
        {
            file->has_video = 0;
            
            dbg_warn("AVERROR_STREAM_NOT_FOUND\n");
        }
        else if(decoder->video_stream_index == AVERROR_DECODER_NOT_FOUND)
        {
            dbg_error("AVERROR_DECODER_NOT_FOUND\n");
        }
    }
    
    // audio
    {
        if(decoder->audio_stream_index >= 0)
        {
            file->has_audio = 1;
            
            dbg_success("av_find_best_steam successful for audio.\n");
        }
        else if(decoder->audio_stream_index == AVERROR_STREAM_NOT_FOUND)
        {
            file->has_audio = 0;
            
            dbg_warn("AVERROR_STREAM_NOT_FOUND\n");
        }
        else if(decoder->audio_stream_index == AVERROR_DECODER_NOT_FOUND)
        {
            dbg_error("AVERROR_DECODER_NOT_FOUND\n");
        }
    }
    
    //subtitles
    {
        if(decoder->subtitle_stream_index >= 0)
        {
            file->has_subtitles = 1;
            
            dbg_success("av_find_best_steam successful for subtitles.\n");
        }
        else if(decoder->subtitle_stream_index == AVERROR_STREAM_NOT_FOUND)
        {
            file->has_subtitles = 0;
            
            dbg_warn("AVERROR_STREAM_NOT_FOUND\n");
        }
        else if(decoder->audio_stream_index == AVERROR_DECODER_NOT_FOUND)
        {
            dbg_error("AVERROR_DECODER_NOT_FOUND\n");
        }
    }
    
    // Init video codec context
    if(file->has_video)
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
        
        decoder->video_time_base = //av_inv_q(
            decoder->format_context->streams[decoder->video_stream_index]->time_base;
        decoder->avg_video_framerate = file->target_time;
        
        decoder->video_stream = decoder->format_context->streams[decoder->video_stream_index];
    }
    
    if(file->has_audio)
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
        
        u32 sample_fmt = decoder->audio_codec_context->sample_fmt;
        u32 fmt = 0;
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
        
        decoder->audio_time_base = av_inv_q(decoder->format_context->streams[decoder->audio_stream_index]->time_base);
        dbg_info("audio_time_base: %d/%d\n", decoder->audio_time_base.num, decoder->audio_time_base.den);
        decoder->audio_stream = decoder->format_context->streams[decoder->audio_stream_index];
    }
    
    RETURN(SUCCESS);
}

internal i32
MediaClose(open_file_info *file, decoder_info *decoder, encoder_info *encoder, output_audio *audio, output_video *video)
{
    // TODO(Val): Should we also stop running the current file?
    FQClose(&audio->queue);
    FQClose(&video->queue);
    
    avformat_free_context(decoder->format_context);
    
    avcodec_free_context(&decoder->audio_codec_context);
    avcodec_free_context(&decoder->video_codec_context);
    
    decoder->audio_codec = NULL;
    decoder->video_codec = NULL;
    
    PQClose(&decoder->queue);
    
    RETURN(SUCCESS);
}

internal void
ProcessAllPacketsAndFillFrameBuffers(decoder_info *decoder, output_video *video, output_audio *audio)
{
    StartTimer("ProcessEverything()");
    wlog(LOG_TRACE, "ProcessEverything(): Processing packs and queueing frames");
    
    while(!PQEmpty(decoder->queue) &&
          !FQFull(video->queue) &&
          !FQFull(audio->queue)) // TODO(Val): Add check to see if file has already been fully read
    {
        int ret = 0;
        
        AVPacket *packet;
        
        load_another_packet:
        ret = PQDequeue(decoder->queue, &packet);
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
        
        i32 media_type = 0;
        AVFrame *frame = av_frame_alloc(); 
        
        ret = ProcessPacket(&frame, &media_type, packet, decoder);
        if(ret == NEED_DATA)
        {
            av_packet_unref(packet);
            goto load_another_packet;
        }
        
        if(media_type == VIDEO)
        {
            process_video_frame(&frame, video, decoder);
            f64 time = frame->pts * av_q2d(decoder->video_time_base);
            //dbg_info("Video frame pts: %lf\n", time);
            if(FQEnqueue(video->queue, frame, time))
            {
                dbg_error("Frame queue full.\n");
            }
        }
        else if(media_type == AUDIO)
        {
            process_audio_frame(&frame, audio, decoder);
            f64 time = frame->pts / av_q2d(decoder->audio_time_base);
            //dbg_info("Audio frame pts: %lf\n", time);
            if(FQEnqueue(audio->queue, frame, time))
            {
                dbg_error("Frame queue full.\n");
            }
        }
        else if(media_type == SUBTITLE)
        {
            // TODO(Val): handle this
        }
        else 
        {
            wlog(LOG_WARNING, "ProcessEverything(): Unknown packet type received");
        }
        
        av_packet_unref(packet);
    }
    
    EndTimer();
}

not_used internal b32
EnoughDurations(output_audio *audio, open_file_info *file, output_video *video, playback_data *playback)
{
    StartTimer("EnoughDurations()");
    
    f64 audio_time = audio->duration;
    // TODO(Val): This will break the application if the framerate is larger than the monitor refresh rate.
    f64 video_time = video->frame_duration;
    f64 refresh_time = *playback->refresh_target;
    f64 next_time = get_future_playback_time(playback);
    
    b32 audio_enough =
        !file->has_audio ||
        (audio->is_ready && ((playback->audio_total_queued + audio->duration) >= next_time)) ||
        (playback->audio_total_queued >= next_time);
    b32 video_enough = !file->has_video || (video->is_ready && ((video->pts + video->frame_duration) >= next_time));
    
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

internal i32
RefillPackets(decoder_info *decoder, b32 is_host)
{
    while(!PQFull(decoder->queue) && !decoder->file_fully_loaded)
    {
        AVPacket *packet;
        LoadPacket(decoder, &packet);
        
        PQEnqueue(decoder->queue, packet);
        
        if(is_host)
            Streaming_Host_SendPacket(decoder, packet);
    }
    RETURN(SUCCESS);
}

internal i32
MediaThreadStart(void *arg)
{
    InitializeTimingSystem("media");
    
    wlog(LOG_INFO, "MediaThreadStart(): Media processing thread started");
    
    program_data *pdata = arg;
    open_file_info *file = &pdata->file;
    decoder_info *decoder = &pdata->decoder;
    //encoder_info *encoder = &pdata->encoder;
    playback_data *playback = &pdata->playback;
    
    if(HostingFile(pdata))
    {
        char address[16];
        IPToStr(address, pdata->address_storage);
        
        Streaming_Host_Initialize(decoder, file, address);
        
        PlatformConditionWait(&decoder->start_streaming);
    }
    
    int ret = 0;
    
    b32 start_notified = 0;
    
    wlog(LOG_INFO, "MediaThreadStart(): Start media processing loop");
    while(pdata->running && !PlaybackFinished(pdata))
    {
        StartTimer("Start media processing loop");
        
        wlog(LOG_TRACE, "Refilling packets");
        // TODO(Val): if not finished reading packets
        RefillPackets(decoder, HostingFile(pdata));
        ProcessAllPacketsAndFillFrameBuffers(decoder, &pdata->video, &pdata->audio);
        
        EndTimer();
        
        if(unlikely(!start_notified) &&
           !FQEmpty(pdata->video.queue) &&
           !FQEmpty(pdata->audio.queue))
        {
            wlog(LOG_INFO, "Playback marked ready to start.\n");
            
            SetPlaybackStartFlag(pdata, 1);
            start_notified = 1;
        }
        
        wlog(LOG_TRACE, "MediaThreadStart(): Sleeping until new frames needed");
        PlatformConditionWait(&decoder->condition);
    }
    wlog(LOG_INFO, "Exiting media processing loop");
    EndTimer();
    
    if(HostingFile(pdata))
        Streaming_Host_Close(decoder);
    
    FinishTiming();
    RETURN(SUCCESS);
}
