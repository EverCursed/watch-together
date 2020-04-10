/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include "streaming.h"

#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>

#include "common/custom_malloc.h"
#include "utils.h"
#include "errors.h"
#include "defines.h"
#include "network.h"
#include "debug.h"
#include "decoding.h"
#include "attributes.h"
#include "file_data.h"

static char* address_prefix  = "tcp";
static char* output_format   = "mpegts";
static int32 video_port      = 16336;
//static char* client_parameters      = "";
static char* host_parameters      = "?listen";

internal int32
Streaming_Client_Initialize(decoder_info *decoder)
{
    wlog(LOG_INFO, "Streaming_Client_Initialize()");
    
    avformat_network_init();
    
    RETURN(UNKNOWN_ERROR);
}

not_used internal AVStream *
add_stream(decoder_info *decoder,
           AVFormatContext *output_format_context,
           AVCodec *codec)
{
    AVCodecContext *codec_context = NULL;
    AVStream *stream;
    stream = avformat_new_stream(output_format_context, codec);
    if (!stream) {
        dbg_error("Could not allocate stream.\n");
        return NULL;
    }
    
    /* Some formats want stream headers to be separate. */
    if(output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    return stream;
}

internal int32
copy_stream_to_output(AVFormatContext *output_fmt_context, AVStream *input_stream)
{
    int ret = 0;
    
    AVStream *out_stream;
    AVCodecParameters *input_params = input_stream->codecpar;
    
    out_stream = avformat_new_stream(output_fmt_context, NULL);
    if (!out_stream) {
        dbg_error("Failed allocating output stream\n");
        return -1;
    }
    
    ret = avcodec_parameters_copy(out_stream->codecpar, input_params);
    if (ret < 0) {
        dbg_error("Failed to copy codec parameters\n");
        return -1;
    }
    out_stream->codecpar->codec_tag = 0;
    
    return out_stream->index;
}

internal int32
Streaming_Host_Initialize(decoder_info *decoder, open_file_info *file, char *my_ip)
{
    wlog(LOG_INFO, "Streamin_Host_Initialize()");
    
    avformat_network_init();
    
    int ret = 0;
    char video_address[256];
    //char audio_address[256];
    //char subtitle_address[256];
    Streaming_GetFileName(video_address, my_ip, video_port, host_parameters);
    
    avformat_alloc_output_context2(&decoder->output_context, NULL, output_format, video_address); //address);
    
    if(!decoder->output_context)
        RETURN(UNKNOWN_ERROR);
    
    decoder->stream_mapping_size = decoder->format_context->nb_streams;
    decoder->stream_mapping = av_mallocz_array(decoder->stream_mapping_size, sizeof(*decoder->stream_mapping));
    for(int i = 0; i < decoder->format_context->nb_streams; i++)
    {
        int temp = copy_stream_to_output(decoder->output_context, decoder->format_context->streams[i]);
        decoder->stream_mapping[i] = temp >= 0 ? temp : -1;
    }
    
    av_dump_format(decoder->output_context, 0, video_address, 1);
    if (!(decoder->output_context->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&decoder->output_context->pb, video_address, AVIO_FLAG_WRITE | AVIO_FLAG_NONBLOCK);
        if (ret < 0) {
            dbg_error("Could not open output file '%s'\n", video_address);
            RETURN(UNKNOWN_ERROR);
        }
    }
    
    if(decoder->output_context->oformat->flags & AVFMT_GLOBALHEADER)
        decoder->output_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    ret = avformat_write_header(decoder->output_context, NULL);
    
    decoder->frames_sent = 0;
    
    RETURN(SUCCESS);
}

internal bool32
Streaming_Host_Is_Initialized(decoder_info *decoder)
{
    return !!decoder->output_context;
}

internal int32
Streaming_Host_Accept(decoder_info *decoder)
{
    //if(Streaming_Host_Is_Initialized(decoder))
    //{
    //
    //}
    
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_Host_Close(decoder_info *decoder)
{
    wlog(LOG_INFO, "Streaming_Host_Close()");
    
    //av_write_trailer(output_fmt_context);
    
    if(decoder->output_context && !(decoder->output_context->oformat->flags & AVFMT_NOFILE))
        avio_close(decoder->output_context->pb);
    avformat_free_context(decoder->output_context);
    
    RETURN(SUCCESS);
}

internal int32
Streaming_Client_Close(decoder_info *decoder)
{
    wlog(LOG_INFO, "Streaming_Client_Close()");
    
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_Accept_Client()
{
    RETURN(UNKNOWN_ERROR);
}

internal int32
Streaming_Client_Connect(char* address)
{
    /*
    AVIODirContext *ctx = NULL;
    char address[256];
    char full_address[256];
    
    get_ip_string(address, ip);
    
    if(ip.is_ipv6)
    sprintf(full_address, "rtp://[%s]:%d", address, port);
    else
    sprintf(full_address, "rtp://%s:%d", address, port);
    
    
    if (avio_open_dir(&ctx, address, NULL) < 0) {
    fprintf(stderr, "Cannot open directory.\n");
    RETURN(UNKNOWN_ERROR);
    }
    */
    
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_GetFileName(char *buffer, char *address, int32 video_port, char *parameters)
{
    sprintf(buffer, "%s://%s:%d%s",
            address_prefix,
            address,
            video_port,
            parameters ? parameters : "");
    
    RETURN(SUCCESS);
}

internal int32
Streaming_Get_Port()
{
    return video_port;
}

internal int32
Streaming_Client_Disconnect()
{
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_Client_GetPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_Host_SendPacket(decoder_info *decoder, AVPacket *packet)
{
    int ret = 0;
    
    AVStream *output_stream = NULL, *input_stream = NULL;
    
    AVPacket *pkt = av_packet_clone(packet); 	
    
    input_stream = decoder->format_context->streams[pkt->stream_index];
    output_stream = decoder->output_context->streams[decoder->stream_mapping[pkt->stream_index]];
    pkt->stream_index = decoder->stream_mapping[pkt->stream_index];
    
    if(pkt->stream_index != -1)
    {
        if(packet->pts & AV_NOPTS_VALUE ||
           packet->dts & AV_NOPTS_VALUE ||
           packet->duration & AV_NOPTS_VALUE)
        {
            dbg_info("Packet timestamps AV_NOPTS_VALUE");
            AVRational time_base1 = decoder->video_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(decoder->video_stream->r_frame_rate);
            //Parameters
            pkt->pts = (double)(decoder->frames_sent*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt->dts = pkt->pts;
            pkt->duration = (double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
            
            //pkt->pts = decoder->frames_sent;
            //pkt->dts = decoder->frames_sent;
            decoder->frames_sent++;
        }
        
        pkt->pts = av_rescale_q_rnd(pkt->pts, input_stream->time_base, output_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->dts = av_rescale_q_rnd(pkt->dts, input_stream->time_base, output_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt->duration = av_rescale_q(pkt->duration, input_stream->time_base, output_stream->time_base);
        pkt->pos = -1;
        
        //ret = avformat_write_header(output_video_context, NULL);
        
        ret = av_interleaved_write_frame(decoder->output_context, pkt);
        FF_Test(ret);
    }
    //ret = av_write_trailer(output_video_context);
    //FF_Test(ret);
    
    av_packet_free(&pkt);
    //av_packet_unref(pkt);
    RETURN(SUCCESS);
}

internal int32
Streaming_GetControlPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
Streaming_SendControlPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}
