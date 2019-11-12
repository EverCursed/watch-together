/*
TODO(Val): 
 - Initialize stream context
 - Close stream context
 - Check connection
- Send packet
 - Send control message
 - Receive packet
 - Receive control message
 
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

static char* connection_type = "rtp";
static char* connection_ip = "localhost";
static char* video_port = "16334";
//static char* audio_port = "16336";
//static char* subtitle_port = "16338";
//static char* connection_file = "live";

static AVFormatContext *output_fmt_context;
static AVOutputFormat *output_fmt;
static AVStream *output_video_stream;
//static AVStream *output_audio_stream;
//static AVStream *output_subtitle_stream;

int32
Streaming_Initialize()
{
    avformat_network_init();
    
    RETURN(SUCCESS);
}

int32
Streaming_Deinitialize()
{
    avformat_network_deinit();
    
    RETURN(SUCCESS);
}

int32
Streaming_Client_Initialize()
{
    
    
    RETURN(UNKNOWN_ERROR);
}

int32
Streaming_Host_Initialize(decoder_info *decoder)
{
    int ret = 0;
    char address[256];
    sprintf(address, "%s://%s:%s", connection_type, connection_ip, video_port); //, connection_file);
    
    //avformat_alloc_output_context2(&output_fmt_context, NULL, "flv", out_filename); //RTMP
	avformat_alloc_output_context2(&output_fmt_context, NULL, "rtp", address);
    
	if (!output_fmt_context) {
		dbg_error("Failed to create output context.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
    output_fmt = output_fmt_context->oformat;
    
    
    output_video_stream = avformat_new_stream(output_fmt_context, decoder->video_codec_context->codec);
    if(!output_video_stream)
    {
        RETURN(UNKNOWN_ERROR);
    }
    
    //output_audio_stream = avformat_new_stream(output_fmt_context, decoder->audio_codec_context->codec);
    //if(!output_audio_stream)
    //{
    //RETURN(UNKNOWN_ERROR);
    //}
    
    AVCodecParameters *video_pars = avcodec_parameters_alloc();
    //AVCodecParameters *audio_pars = avcodec_parameters_alloc();
    avcodec_parameters_from_context(video_pars, decoder->video_codec_context);
    //avcodec_parameters_from_context(audio_pars, decoder->audio_codec_context);
    avcodec_parameters_copy(output_video_stream->codecpar, video_pars);
    //avcodec_parameters_copy(output_audio_stream->codecpar, audio_pars);
    avcodec_parameters_free(&video_pars);
    //avcodec_parameters_free(&audio_pars);
    
    // TODO(Val): Check if we need to do this
    /*
    output_video_stream->codec->codec_tag = 0;
    if (output_fmt_context->oformat->flags & AVFMT_GLOBALHEADER)
        output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    */
    
    //Copy the settings of AVCodecContext
    av_dump_format(output_fmt_context, 0, address, 1);
    
    if (!(output_fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_fmt_context->pb, address, AVIO_FLAG_WRITE);
        if (ret < 0) {
            dbg_error( "Could not open output URL '%s'", address);
            RETURN(UNKNOWN_ERROR);
        }
    }
    
    ret = avformat_write_header(output_fmt_context, NULL);
    if(ret == AVSTREAM_INIT_IN_WRITE_HEADER)
    {
        
    }
    else if(ret == AVSTREAM_INIT_IN_INIT_OUTPUT)
    {
        
    }
    
    
    
    RETURN(SUCCESS);
}

int32
Streaming_Host_Close()
{
    
    if (output_fmt_context && !(output_fmt->flags & AVFMT_NOFILE))
        avio_close(output_fmt_context->pb);
    avformat_free_context(output_fmt_context);
    
    
    av_write_trailer(output_fmt_context);
    
    
    RETURN(SUCCESS);
}

int32
Streaming_Accept_Client()
{
    RETURN(UNKNOWN_ERROR);
}

int32
Streaming_Client_Close()
{
    RETURN(UNKNOWN_ERROR);
}

int32
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

int32
Streaming_Client_Disconnect()
{
    RETURN(NOT_YET_IMPLEMENTED);
}


int32
Streaming_Client_GetPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}

int32
Streaming_Host_SendPacket(decoder_info *decoder, AVPacket *packet)
{
    int ret = 0;
    //FIX No PTS (Example: Raw H.264)
    //Simple Write PTS
    /*
    if(pkt.pts==AV_NOPTS_VALUE){
    //Write PTS
    AVRational time_base1=ifmt_ctx->streams[videoindex]->time_base;
    //Duration between 2 frames (us)
    int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
    //Parameters
    pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    pkt.dts=pkt.pts;
    pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    }
    */
    //in_stream  = ifmt_ctx->streams[pkt.stream_index];
    //out_stream = ofmt_ctx->streams[pkt.stream_index];
    
    /* copy packet */
    //Convert PTS/DTS
    //pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    //pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    //pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    //pkt.pos = -1;
    //Print to Screen
    /*
    if(pkt.stream_index==videoindex){
    printf("Send %8d video frames to output URL\n",frame_index);
    frame_index++;
    }
    */
    //ret = av_write_frame(ofmt_ctx, &pkt);
    
    /*
    ret = avformat_write_header(output_fmt_context, NULL);
    if(ret == AVSTREAM_INIT_IN_WRITE_HEADER)
    {
    
    }
    else if(ret == AVSTREAM_INIT_IN_INIT_OUTPUT)
    {
    
    }
    */
    
    /* copy packet */
    //Convert PTS/DTS
    
    if(packet->stream_index == decoder->video_stream_index)
    {
        int old_index = packet->stream_index;
        packet->stream_index = 0;
        
        packet->pts = av_rescale_q_rnd(packet->pts, decoder->video_stream->time_base, output_video_stream->time_base, (AV_ROUND_ZERO|AV_ROUND_PASS_MINMAX));
        packet->dts = av_rescale_q_rnd(packet->dts, decoder->video_stream->time_base, output_video_stream->time_base, (AV_ROUND_ZERO|AV_ROUND_PASS_MINMAX));
        packet->duration = av_rescale_q(packet->duration, decoder->video_stream->time_base, output_video_stream->time_base);
        packet->pos = -1;
        //Print to Screen
        
        ret = av_interleaved_write_frame(output_fmt_context, packet);
        
        //av_write_trailer(output_fmt_context);
        packet->stream_index = old_index;
        
        if (ret < 0) {
            dbg_error("Error muxing packet\n");
            RETURN(UNKNOWN_ERROR);
        }
        
        av_packet_unref(packet);
    }
    
    RETURN(NOT_YET_IMPLEMENTED);
}

int32
Streaming_GetControlPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}

int32
Streaming_SendControlPacket()
{
    RETURN(NOT_YET_IMPLEMENTED);
}
