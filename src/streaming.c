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
#include "attributes.h"

static char* connection_type = "rtp";
static char* connection_ip = "127.0.0.1";
static char* video_port = "16334";
//static char* audio_port = "16336";
//static char* subtitle_port = "16338";
//static char* connection_file = "live";

static AVFormatContext *output_fmt_context;
static AVOutputFormat *output_fmt;
static AVStream *output_video_stream;
static AVStream *output_audio_stream;
static AVStream *output_subtitle_stream;

static char* sdp_file;
static int sdp_size;

#define SDP_FILE_MAXSIZE 1024

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

not_used static AVStream *
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

static AVStream *
copy_stream_to_output(AVFormatContext *output_fmt_context, AVStream *input_stream)
{
    int ret = 0;
    
    AVStream *out_stream;
    AVCodecParameters *input_params = input_stream->codecpar;
    
    out_stream = avformat_new_stream(output_fmt_context, NULL);
    if (!out_stream) {
        dbg_error("Failed allocating output stream\n");
        return NULL;
    }
    ret = avcodec_parameters_copy(out_stream->codecpar, input_params);
    if (ret < 0) {
        dbg_error("Failed to copy codec parameters\n");
        return NULL;
    }
    out_stream->codecpar->codec_tag = 0;
    
    return out_stream;
}

int32
Streaming_Host_Initialize(decoder_info *decoder)
{
    int ret = 0;
    int stream_count = 0;
    char address[256];
    sprintf(address, "%s://%s:%s", connection_type, connection_ip, video_port); //, connection_file);
    
    //int ret, i;
    //int stream_index = 0;
    //int *stream_mapping = NULL;
    //int stream_mapping_size = 0;
    
    avformat_alloc_output_context2(&output_fmt_context, NULL, "mpegts", NULL); //address);
    if (!output_fmt_context) {
        dbg_error("Could not create output context.\n");
        RETURN(UNKNOWN_ERROR);
    }
    //stream_mapping_size = ifmt_ctx->nb_streams;
    //stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    //if (!stream_mapping) {
    //ret = AVERROR(ENOMEM);
    //goto end;
    //}
    output_fmt = output_fmt_context->oformat;
    
    if(decoder->video_stream)
    {
        decoder->output_video_stream = copy_stream_to_output(output_fmt_context, decoder->video_stream);
        if(!output_video_stream)
            dbg_error("Failed to create a video stream.\n");
        decoder->output_video_stream_index = stream_count++;
    }
    if(decoder->audio_stream)
    {
        decoder->output_audio_stream = copy_stream_to_output(output_fmt_context, decoder->audio_stream);
        if(!output_audio_stream)
            dbg_error("Failed to create a audio stream.\n");
        decoder->output_audio_stream_index = stream_count++;
    }
    if(decoder->subtitle_stream)
    {
        decoder->output_subtitle_stream = copy_stream_to_output(output_fmt_context, decoder->subtitle_stream);
        if(!output_subtitle_stream)
            dbg_error("Failed to create a subtitle stream.\n");
        decoder->output_subtitle_stream_index = stream_count++;
    }
    
    av_dump_format(output_fmt_context, 0, address, 1);
    if (!(output_fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_fmt_context->pb, address, AVIO_FLAG_WRITE);
        if (ret < 0) {
            dbg_error("Could not open output file '%s'", address);
            RETURN(UNKNOWN_ERROR);
        }
    }
    //ret = avformat_write_header(output_fmt_context, NULL);
    /*
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
    while (1) {
        }
    av_write_trailer(ofmt_ctx);
    end:
    avformat_close_input(&ifmt_ctx);
    // close output
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    av_freep(&stream_mapping);
    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }
    return 0;
*/
    
    RETURN(SUCCESS);
}

/*
int32
Streaming_Host_Initialize(decoder_info *decoder)
{
int ret = 0;
char address[256];
sprintf(address, "%s://%s:%s", connection_type, connection_ip, video_port); //, connection_file);

output_fmt->video_codec = decoder->video_codec_context->codec_id;
output_fmt->audio_codec = decoder->audio_codec_context->codec_id;
output_fmt->subtitle_codec = decoder->subtitle_codec_context->codec_id;

//avformat_alloc_output_context2(&output_fmt_context, NULL, "flv", out_filename); //RTMP
avformat_alloc_output_context2(&output_fmt_context, output_fmt, NULL, address);
if (!output_fmt_context) {
dbg_error("Failed to create output context.\n");
RETURN(UNKNOWN_ERROR);
}
output_fmt = output_fmt_context->oformat;

if (output_fmt->video_codec != AV_CODEC_ID_NONE) {
output_video_stream = add_stream(decoder, output_fmt_context, decoder->video_codec);

decoder->output_video_codec_context = avcodec_alloc_context3(decoder->video_codec);
if(!decoder->output_video_codec_context)
{
dbg_error("Could not allocate codec context.\n");
return NULL;
}


}
if (output_fmt->audio_codec != AV_CODEC_ID_NONE) {
output_audio_stream = add_stream(decoder, output_fmt_context, decoder->audio_codec);
}
//if (output_fmt->subtitle_codec != AV_CODEC_ID_NONE) {
//add_stream(decoder, output_fmt_context, decoder->subtitle_codec);
//}

avcodec_parameters_from_context(output_video_stream->codecpar, decoder->video_codec_context);
avcodec_parameters_from_context(output_audio_stream->codecpar, decoder->audio_codec_context);

// TODO(Val): Check if we need to do this
//
output_video_stream->codec->codec_tag = 0;
if (output_fmt_context->oformat->flags & AVFMT_GLOBALHEADER)
output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
//

//Copy the settings of AVCodecContext
av_dump_format(output_fmt_context, 0, address, 1);
//
if (!(output_fmt->flags & AVFMT_NOFILE)) {
    ret = avio_open(&output_fmt_context->pb, address, AVIO_FLAG_WRITE);
    if (ret < 0) {
        dbg_error( "Could not open output URL '%s'", address);
        RETURN(UNKNOWN_ERROR);
    }
}
//
ret = avformat_write_header(output_fmt_context, NULL);
if(ret == AVSTREAM_INIT_IN_WRITE_HEADER)
{

}
else if(ret == AVSTREAM_INIT_IN_INIT_OUTPUT)
{

}



if(sdp_file)
custom_free(sdp_file);
sdp_file = custom_malloc(SDP_FILE_MAXSIZE);

ret = av_sdp_create(&output_fmt_context,
                    1,
                    sdp_file,
                    SDP_FILE_MAXSIZE);
FF_Test(ret);

dbg_info("SDP file:\n"
         "%s\n",
         sdp_file);
         
FF_Test(ret);


RETURN(SUCCESS);
}
*/

int32
Streaming_Host_Close()
{
    //av_write_trailer(output_fmt_context);
    
    if (output_fmt_context && !(output_fmt->flags & AVFMT_NOFILE))
        avio_close(output_fmt_context->pb);
    avformat_free_context(output_fmt_context);
    
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
    
    AVStream *output_stream = NULL, *input_stream = NULL;
    
    if(packet->stream_index == decoder->video_stream_index)
    {
        packet->stream_index = decoder->output_video_stream_index;
        output_stream = decoder->output_video_stream;
        input_stream = decoder->video_stream;
    }
    else if(packet->stream_index == decoder->audio_stream_index)
    {
        packet->stream_index = decoder->output_audio_stream_index;
        output_stream = decoder->output_audio_stream;
        input_stream = decoder->audio_stream;
    }
    else if(packet->stream_index == decoder->subtitle_stream_index)
    {
        packet->stream_index = decoder->output_subtitle_stream_index;
        output_stream = decoder->output_subtitle_stream;
        input_stream = decoder->subtitle_stream;
    }
    else
        RETURN(SUCCESS);
    
    AVPacket *pkt = av_packet_clone(packet); 	
    // copy packet
    //pkt->buf = packet->buf;
    //pkt->data = packet->data;
    //pkt->size = packet->size;
    //pkt->flags = packet->flags;
    pkt->pts = av_rescale_q_rnd(packet->pts, input_stream->time_base, output_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->dts = av_rescale_q_rnd(packet->dts, input_stream->time_base, output_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->duration = av_rescale_q(pkt->duration, input_stream->time_base, output_stream->time_base);
    pkt->pos = -1;
    
    ret = avformat_write_header(output_fmt_context, NULL);
    
    ret = av_interleaved_write_frame(output_fmt_context, pkt);
    FF_Test(ret);
    
    ret = av_write_trailer(output_fmt_context);
    FF_Test(ret);
    
    av_packet_free(&pkt);
    //av_packet_unref(pkt);
    
    /*
    
    
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
    */
    RETURN(SUCCESS);
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

int32
Streaming_GetSDP(char **file, int *size)
{
    if(sdp_file)
    {
        *file = sdp_file;
        *size = sdp_size;
        
        RETURN(SUCCESS);
    }
    
    RETURN(UNKNOWN_ERROR);
}
