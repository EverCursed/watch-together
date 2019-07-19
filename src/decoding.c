#include <libswscale/version.h>
#include <libswscale/swscale.h>

#include "defines.h"
#include "watchtogether.h"
#include "platform.h"
#include "packet_queue.h"
#include "decoding.h"

static void
file_close(program_data *pdata)
{
    // TODO(Val): Should we also stop running the current file?
    
    decoder_info *decoder = &pdata->decoder;
    open_file_info *file = &pdata->file;
    
    avformat_free_context(decoder->format_context);
    
    avcodec_free_context(&decoder->audio_codec_context);
    avcodec_free_context(&decoder->video_codec_context);
    
    decoder->audio_codec = NULL;
    decoder->video_codec = NULL;
    
    memset(file, 0, sizeof(open_file_info));
    memset(decoder, 0, sizeof(decoder_info));
}

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

static int32
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

static struct frame_info
get_frame(program_data *pdata, int32 stream)
{
    decoder_info *decoder = &pdata->decoder;
    AVCodecContext *dec_ctx;
    
    AVPacket *pkt;// = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    struct frame_info info = {.frame = frame, .type = 0, .ret = -1};
    
    if(!frame) {
        dbg_error("av_frame_alloc() failed!\n");
        return info;
    }
    
    int32 ret = 0;
    avpacket_queue *queue = NULL;
    
    // set the type of frame.
    if(stream == decoder->video_stream)
    {
        // TODO(Val): Temporarily converting everything to YUV
        info.type = FRAME_VIDEO;
        
        //printf("frame->format == %d\n", format_context->streams[video_stream]->codecpar->format);
        /*
        if(decoder->format_context->streams[decoder->video_stream]->codecpar->format == AV_PIX_FMT_YUV420P)
        {
            info.type = FRAME_VIDEO_YUV;
        }
        else
        {
            info.type = FRAME_VIDEO_RGB;
        }
        */
        queue = pdata->pq_video;
        dec_ctx = decoder->video_codec_context;
    }
    else if(stream == decoder->audio_stream)
    {
        info.type = FRAME_AUDIO;
        
        queue = pdata->pq_audio;
        dec_ctx = decoder->audio_codec_context;
    }
    else
    {
        dbg_error("Unknown frame returned.\n");
        info.type = FRAME_UNKNOWN;
        goto get_frame_failed;
    }
    
    do {
        ret = peek_packet(queue, &pkt, 0);
        if(ret == -1)
            goto get_frame_failed;
        
        ret = avcodec_send_packet(dec_ctx, pkt);
        dbg_info("avcodec_send_packet\tret: %d\n", ret);
        
        if(ret == AVERROR(EAGAIN))
        {
            dbg_error("AVERROR(EAGAIN)\n");
            // NOTE(Val): This means a frame must be read before we can send another packet
        }
        else if(ret == AVERROR_EOF)
        {
            pdata->file.file_finished = 1;
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
        dbg_info("avcodec_receive_frame\tret: %d\n", ret);
        
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
    dbg_success("Returning from decoding.\n");
    return info;
    
    get_frame_failed:
    av_packet_unref(pkt);
    av_frame_free(&frame);
    info.ret = -1;
    return info;
    /*
    send_packet:
    // TODO(Val): This needs to be changed. Getting called here again would break.
    if(ret == AVERROR(EAGAIN))
    {
    dbg_error("AVERROR(EAGAIN) returned.\n");
    av_packet_unref(pkt);
    
    if(info.type == FRAME_AUDIO)
    {
    dequeue_packet(pdata->pq_audio, &pkt);
    }
    else if(info.type == FRAME_VIDEO_RGB ||
    info.type == FRAME_VIDEO_YUV)
    {
    dequeue_packet(pdata->pq_video, &pkt);
    }
    
    goto send_packet;
    }
    else if(ret == AVERROR_EOF ||
    ret == AVERROR(EINVAL) ||
    ret == AVERROR(ENOMEM)) 
    {
    if(ret == AVERROR(EINVAL))
    dbg_error("EINVAL\n");
    else
    dbg_error("Error sending a packet for decoding\n");
    goto wt_decode_failed;
    }
    else if(ret == 0)
    {
    dbg_success("avcodec_send_packet() succeeded.\n");
    }
    
    if(ret == AVERROR(EAGAIN))
    {
    dbg_error("AVERROR(EAGAIN)\n");
    
    if(info.type == FRAME_AUDIO)
    {
    dequeue_packet(pdata->pq_audio, &pkt);
    }
    else if(info.type == FRAME_VIDEO_RGB ||
    info.type == FRAME_VIDEO_YUV)
    {
    dequeue_packet(pdata->pq_video, &pkt);
    }
    
    goto send_packet;
    }
    else if(ret == AVERROR(EINVAL))
    {
    dbg_error("AVERROR(EINVAL)\n");
    goto wt_decode_failed;
    }
    else if(ret == AVERROR_EOF)
    {
    dbg_error("AVERROR_EOF\n");
    // TODO(Val): End of file, do something here.
    goto wt_decode_failed;
    }
    else 
    {
    info.frame = frame;
    }
    
    return info;
    */
}

static int32
file_open(open_file_info *file, decoder_info *decoder)
{
    // Open video file
    dbg_print("avformat version: %d - %d\n", LIBAVFORMAT_VERSION_INT, avformat_version());
    
    int ret = 0;
    //decoder->format_context = avformat_alloc_context();
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
        file->target_time = (real32)time.den/(real32)time.num * 1000.0f;
        
        file->has_video = 1;
        
        decoder->video_time_base = //av_inv_q(
            decoder->format_context->streams[decoder->video_stream]->time_base;
        //file->video_stream = decoder->video_stream;
        //file->video_time_base = decoder->format_context->streams[video_stream].time_base;
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
        
        file->sample_rate = decoder->audio_codec_context->sample_rate;
        file->bytes_per_sample = bytes_per_sample;
        file->channels = decoder->audio_codec_context->channels;
        file->sample_format = fmt;
        
        file->has_audio = 1;
        //decoder->audio_time_base = decoder->audio_codec_context->framerate;
        
        decoder->audio_time_base = av_inv_q(decoder->format_context->streams[decoder->audio_stream]->avg_frame_rate);
    }
    
    return 0;
}

#define AUDIO_FRAME 1
#define VIDEO_FRAME 2

static void
copy_pixel_buffers(void *dst,
                   void *src,
                   int32 pitch_dst,
                   int32 pitch_src,
                   int32 height)
{
    for(int i = 0; i < height; i++)
    {
        memcpy(dst + i*pitch_dst, src + i*pitch_src, pitch_src);
    }
}

global struct SwsContext* modifContext = NULL;

static int32
process_video_frame(program_data *pdata, struct frame_info info)
{
    uint32 ret = 0;
    AVFrame *frame = info.frame;
    decoder_info *decoder = &pdata->decoder;
    
    dbg_info("Start processing video frame.\n");
    
    
    /*
    
    if(pdata->running && info.type == FRAME_VIDEO_RGB)
    {
    modifContext = sws_getCachedContext(modifContext,
    decoder->video_codec_context->width,
    decoder->video_codec_context->height,
    frame->format,
    decoder->video_codec_context->width,  // dst width
    decoder->video_codec_context->height, // dst height
    //video_buffer->width,
    //video_buffer->height,
    AV_PIX_FMT_RGB32,
    SWS_BICUBIC,
    NULL, NULL, NULL);
    
    int32 pitch = round_up_align(decoder->video_codec_context->width *
    av_get_bits_per_pixel(
    av_pix_fmt_desc_get(
    decoder->video_codec_context->pix_fmt)));
    
    // TODO(Val): This malloc is wrong. Better to allocate buffer
    // before, and then reuse it and free at the end.
    void *temp_buffer = malloc(pitch * decoder->video_codec_context->height);
    uint8_t *ptrs[1] = { temp_buffer };
    int stride[1] = { pitch };
    
    sws_scale(modifContext,
    (uint8 const* const*)frame->data,
    frame->linesize,
    0,
    decoder->video_codec_context->height,
    (uint8 *const *const)ptrs,
    stride);
    
    pdata->video.video_frame = temp_buffer;
    pdata->video.time = frame->pts;
    //pdata->video.duration = frame->duration;
    pdata->video.pitch = stride[0];
    pdata->video.width = frame->width;
    pdata->video.height = frame->height;
    pdata->video.type = VIDEO_RGB;
    pdata->video.is_ready = 1;
    
    //while(pdata->running && (enqueue_frame(&pdata->vq_data, temp_buffer, pdata->vq_data.vq_pitch, frame->pts) < 0))
    //{
    //SDL_Delay((int32)pdata->file.target_time);
    //}
    //free(temp_buffer);
    //}
    else if(pdata->running && info.type == FRAME_VIDEO_YUV)
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
    
    int32 pitch_Y = round_up_align(frame->width *
    av_get_bits_per_pixel(
    av_pix_fmt_desc_get(
    frame->format)));
    int32 pitch_U = round_up_align((frame->width+1)/2 *
    av_get_bits_per_pixel(
    av_pix_fmt_desc_get(
    frame->format)));
    int32 pitch_V = round_up_align((frame->width+1)/2 *
    av_get_bits_per_pixel(
    av_pix_fmt_desc_get(
    frame->format)));
    
    void *frame_Y = malloc(pitch_Y * frame->height);
    void *frame_U = malloc(pitch_U * (frame->height+1)/2);
    void *frame_V = malloc(pitch_V * (frame->height+1)/2);
    
    copy_pixel_buffers(frame_Y, frame->data[0], pitch_Y, frame->linesize[0], frame->height);
    copy_pixel_buffers(frame_U, frame->data[1], pitch_U, frame->linesize[1], (frame->height+1)/2);
    copy_pixel_buffers(frame_V, frame->data[2], pitch_V, frame->linesize[2], (frame->height+1)/2);
    
    pdata->video.video_frame = frame_Y;
    pdata->video.video_frame_sup1 = frame_U;
    pdata->video.video_frame_sup2 = frame_V;
    
    pdata->video.time = frame->pts;
    //pdata->video.duration = frame->duration;
    
    pdata->video.pitch = pitch_Y;
    pdata->video.pitch_sup1 = pitch_U;
    pdata->video.pitch_sup2 = pitch_V;
    
    pdata->video.width = frame->width;
    pdata->video.height = frame->height;
    pdata->video.type = VIDEO_YUV;
    pdata->video.is_ready = 1;
    
    //while(pdata->running && (enqueue_frame_YUV(&pdata->vq_data,
    //frame->data[0],
    //frame->linesize[0],
    //frame->data[1],
    //frame->linesize[1],
    //frame->data[2],
    //frame->linesize[2],
    //frame->pts)))
    //{
    //SDL_Delay((int32)pdata->file.target_time);
    //}
    //
    //}
    //else
    //{
    //dbg_error("Unknown video frame type.\n");
    //}
    */
    
    dbg_warn("frame->format: %d\n"
             "new format   : %d\n",
             frame->format,
             AV_PIX_FMT_YUV420P);
    
    int32 pitch_Y = round_up_align(frame->width *
                                   av_get_bits_per_pixel(
        av_pix_fmt_desc_get(
        frame->format)));
    int32 pitch_U = round_up_align((frame->width+1)/2 *
                                   av_get_bits_per_pixel(
        av_pix_fmt_desc_get(
        frame->format)));
    int32 pitch_V = round_up_align((frame->width+1)/2 *
                                   av_get_bits_per_pixel(
        av_pix_fmt_desc_get(
        frame->format)));
    
    void *frame_Y = malloc(pitch_Y * frame->height);
    void *frame_U = malloc(pitch_U * (frame->height+1)/2);
    void *frame_V = malloc(pitch_V * (frame->height+1)/2);
    
    
    if(frame->format != AV_PIX_FMT_YUV420P)
    {
        modifContext = sws_getCachedContext(modifContext,
                                            decoder->video_codec_context->width,
                                            decoder->video_codec_context->height,
                                            frame->format,
                                            decoder->video_codec_context->width,  // dst width
                                            decoder->video_codec_context->height, // dst height
                                            //video_buffer->width,
                                            //video_buffer->height,
                                            AV_PIX_FMT_YUV420P,
                                            SWS_BICUBIC,
                                            NULL, NULL, NULL);
        
        uint8_t *ptrs[3] = { frame_Y, frame_U, frame_V };
        int stride[3] = { pitch_Y, pitch_U, pitch_V };
        
        sws_scale(modifContext,
                  (uint8 const* const*)frame->data,
                  frame->linesize,
                  0,
                  decoder->video_codec_context->height,
                  (uint8 *const *const)ptrs,
                  stride);
    }
    else
    {
        copy_pixel_buffers(frame_Y,
                           frame->data[0],
                           pitch_Y,
                           frame->linesize[0],
                           frame->height);
        copy_pixel_buffers(frame_U,
                           frame->data[1],
                           pitch_U,
                           frame->linesize[1],
                           (frame->height+1)/2);
        copy_pixel_buffers(frame_V,
                           frame->data[2],
                           pitch_V,
                           frame->linesize[2],
                           (frame->height+1)/2);
    }
    //copy_pixel_buffers(frame_Y, frame->data[0], pitch_Y, frame->linesize[0], frame->height);
    //copy_pixel_buffers(frame_U, frame->data[1], pitch_U, frame->linesize[1], (frame->height+1)/2);
    //copy_pixel_buffers(frame_V, frame->data[2], pitch_V, frame->linesize[2], (frame->height+1)/2);
    
    pdata->video.video_frame = frame_Y;
    pdata->video.video_frame_sup1 = frame_U;
    pdata->video.video_frame_sup2 = frame_V;
    
    //pdata->video.time = frame->pts;
    //pdata->video.duration = frame->duration;
    
    pdata->video.pitch = pitch_Y;
    pdata->video.pitch_sup1 = pitch_U;
    pdata->video.pitch_sup2 = pitch_V;
    
    pdata->video.width = frame->width;
    pdata->video.height = frame->height;
    pdata->video.type = VIDEO_YUV;
    pdata->video.pts = frame->pts * av_q2d(pdata->decoder.video_time_base); 
    dbg_info("Video pts: %lf\n", pdata->video.pts);
    
    pdata->video.is_ready = 1;
    
    /*
    pdata->video.video_frame = temp_buffer_Y;
    pdata->video.video_frame_sup1 = temp_buffer_U;
    pdata->video.video_frame_sup1 = temp_buffer_V;
    pdata->video.time = frame->pts;
    //pdata->video.duration = frame->duration;
    pdata->video.pitch = stride[0];
    pdata->video.width = frame->width;
    pdata->video.height = frame->height;
    pdata->video.type = VIDEO_YUV;
    pdata->video.is_ready = 1;
    */
    
    return 0;
}

static int32
process_audio_frame(program_data *pdata, struct frame_info info)
{
    if(pdata->audio.is_ready)
    {
        dbg_warn("Started processing an audio frame, while the previous one hasn't been used.\n");
        return -1;
    }
    
    uint32 ret = 0;
    AVFrame *frame = info.frame;
    decoder_info *decoder = &pdata->decoder;
    
    uint32 size = av_samples_get_buffer_size(NULL,
                                             decoder->audio_codec_context->channels,
                                             frame->nb_samples,
                                             decoder->audio_codec_context->sample_fmt,
                                             1);
    //dbg_print("Audio buffer size: %d\n", size);
    
    uint32 real_size = frame->linesize[0] * decoder->audio_codec_context->channels; 
    //dbg_print("Size should be: %d\n", real_size);
    
    uint32 SampleCount = frame->nb_samples;
    uint32 Frequency = decoder->audio_codec_context->sample_rate;
    uint32 Channels = decoder->audio_codec_context->channels;
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
    uint32 sample_fmt = decoder->audio_codec_context->sample_fmt; 
    uint32 bytes_per_sample = frame->linesize[0]/frame->nb_samples;
    
    // TODO(Val): Make sure this is deallocated
    void *data = malloc(real_size);
    
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
        memcpy(data, frame->data[0], real_size);
    else
    {
        // NOTE(Val): manually interleaving audio for however
        // many channels
        uint8* dst = data;
        uint32 channels = decoder->audio_codec_context->channels;
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
    
    pdata->audio.buffer = data;
    //pdata->audio.time = frame->pts;
    pdata->audio.size = real_size;
    //pdata->audio.
    pdata->audio.duration = (real64)SampleCount / (real64)Frequency;
    dbg_info("Audio frame duration: %lf\n", pdata->audio.duration);
    
    //pdata->audio.pts = frame->pts * av_q2d(pdata->decoder.audio_time_base); 
    
    //dbg_info("Audio pts: %lf\n", pdata->audio.pts);
    
    pdata->audio.is_ready = 1;
    /*
    while(pdata->running && (enqueue_audio_bytes(&pdata->aq_data, data, size) < 0))
    {
    PlatformSleep(5);
    }
    free(data);
    */
    return 0;
}

#define SLEEP_DURATION 5

// decoding loop, only here temporarily

static void
SortPackets(program_data *pdata)
{
    AVPacket *pkt;
    // NOTE(Val): if there are packets in main queue and audio/video queues aren't full
    while(!pq_is_empty(pdata->pq_main) &&
          !pq_is_full(pdata->pq_audio) &&
          !pq_is_full(pdata->pq_video))
    {
        int ret = dequeue_packet(pdata->pq_main, &pkt);
        
        if(pkt->stream_index == pdata->decoder.video_stream)
        {
            dbg_info("Queued video packet.\n");
            enqueue_packet(pdata->pq_video, pkt);
        }
        else if(pkt->stream_index == pdata->decoder.audio_stream)
        {
            dbg_info("Queued audio packet.\n");
            enqueue_packet(pdata->pq_audio, pkt);
        }
        else
        {
            dbg_info("Discarded unknown packet.\n");
        }
        
        pkt = NULL;
        //av_packet_unref(&pkt);
    }
}

static void
LoadPackets(program_data *pdata)
{
    while(!pq_is_full(pdata->pq_main) &&
          !pdata->file.file_finished) // TODO(Val): check if there are no more packets to load (EOF)
    {
        AVPacket *pkt = av_packet_alloc();
        
        int ret = get_packet(pdata, pkt);
        
        if(ret >= 0)
        {
            enqueue_packet(pdata->pq_main, pkt);
            //av_packet_unref(pkt);
        }
        else
        {
            dbg_error("get_packet() failed.\n");
            break;
        }
    }
    
    if(pq_is_empty(pdata->pq_main))
        pdata->file.file_finished = 1;
}

static void
init_queues(program_data *pdata)
{
    open_file_info *file = &pdata->file;
    
    if(file->has_video)
    {
        if(file->video_format == VIDEO_YUV)
        {
            dbg_info("Initializing YUV queue.\n");
            //init_video_queue_YUV(&pdata->vq_data,
            //pdata->file.width,
            //pdata->file.height);
        }
        else
        {
            dbg_info("Initializing RGB queue.\n");
            //reset_video_queue(&pdata->vq_data,
            //pdata->file.width,
            //pdata->file.height,
            //4);
        }
        
        PlatformInitVideo(pdata);
    }
    
    if(file->has_audio)
    {
        //reset_audio_queue(&pdata->aq_data,
        //pdata->file.sample_rate,
        //pdata->file.channels,
        //pdata->file.bytes_per_sample);
        
        PlatformInitAudio(pdata);
    }
    
    dbg_success("Audio and video queues initialized.\n");
}

static void
close_queues(program_data *pdata)
{
    open_file_info *file = &pdata->file;
    
    if(file->has_video)
    {
        if(file->video_format == VIDEO_YUV)
        {
            dbg_info("Closing YUV queue.\n");
            //close_video_queue_YUV(&pdata->vq_data);
        }
        else
        {
            dbg_info("Closing RGB queue.\n");
            //close_video_queue(&pdata->vq_data);
        }
    }
    
    if(file->has_audio)
    {
        PlatformCloseAudio(pdata);
    }
    
    // TODO(Val): Should I uninitialize audio/video here? 
    // NOTE(Val): Probably no need to close video, as UI will still need to be displayed
    // NOTE(Val): Not sure about audio yet
}

static int32 
DecodingThreadStart(void *ptr)
{
    program_data *pdata = ptr;
    open_file_info *file = &pdata->file;
    decoder_info *decoder = &pdata->decoder;
    
    //init_queues(pdata);
    
    /*
    int ret = file_open(&pdata->file, &pdata->decoder);
    if(ret < 0)
    {
    pdata->file.open_failed = 1;
    
    return -1;
    }
    */
    // TODO(Val): Should this maybe be done in the main thread?
    //init_queues(pdata);
    
    LoadPackets(pdata);
    //print_packets(pdata->pq_main, pdata);
    //print_packets(pdata->pq_video, pdata);
    //print_packets(pdata->pq_audio, pdata);
    
    SortPackets(pdata);
    //print_packets(pdata->pq_main, pdata);
    //print_packets(pdata->pq_video, pdata);
    //print_packets(pdata->pq_audio, pdata);
    
    pdata->start_playback = 1;
    
    while(pdata->running && !pdata->playback_finished)
    {
        dbg_success("Processing loop start.\n");
        if(!pdata->audio.is_ready)
        {
            dbg_success("Audio not marked ready.\n");
            if(!pq_is_empty(pdata->pq_audio))
            {
                dbg_success("Audio packets not empty, starting to process.\n");
                
                struct frame_info f = get_frame(pdata, pdata->decoder.audio_stream);
                
                if(f.ret != -1)
                {
                    dbg_success("Processing audio.\n");
                    
                    process_audio_frame(pdata, f);
                    av_frame_free(&f.frame);
                }
            }
        }
        
        if(!pdata->video.is_ready)
        {
            dbg_success("Video not marked ready.\n");
            if(!pq_is_empty(pdata->pq_video))
            {
                //AVPacket *packet;
                //dequeue_packet(pdata->pq_video, &packet);
                dbg_success("Video packets not empty, starting to process.\n");
                
                struct frame_info f = get_frame(pdata, pdata->decoder.video_stream);
                
                dbg_info("frame type: %d\n", f.frame->format); 
                
                if(f.ret != -1)
                {
                    dbg_success("Processing video.\n");
                    
                    process_video_frame(pdata, f);
                    av_frame_free(&f.frame);
                }
                else
                {
                    dbg_error("get_frame failed.\n");
                }
            }
        }
        
        LoadPackets(pdata);
        //print_packets(pdata->pq_main, pdata);
        //print_packets(pdata->pq_video, pdata);
        //print_packets(pdata->pq_audio, pdata);
        
        SortPackets(pdata);
        //print_packets(pdata->pq_main, pdata);
        //print_packets(pdata->pq_video, pdata);
        //print_packets(pdata->pq_audio, pdata);
        
        if(pq_is_empty(pdata->pq_video) && pq_is_empty(pdata->pq_audio))
            pdata->playback_finished = 1;
        else
            PlatformSleep(10);
    }
    /*
    
    if(!pq_is_empty(pdata->pq_audio) &&
    !pq_is_empty(pdata->pq_video))
    {
    dbg_info("!pq_is_empty()\n");
    AVPacket* pkt[2];
    int32 ret0 = peek_packet(pdata->pq_audio, &pkt[0], 0);
    int32 ret1 = peek_packet(pdata->pq_video, &pkt[1], 0);
    
    int32 soonest_dts = -1;
    if(!ret0 && !ret1)
    {
    soonest_dts = pkt[0]->pts < pkt[1]->pts ?
    (pkt[0]->pts != AV_NOPTS_VALUE ? 0 : -1) :
    (pkt[1]->pts != AV_NOPTS_VALUE ? 1 : -1);
    }
    else if(!ret0)
    {
    soonest_dts = 0;
    }
    else if(!ret1)
    {
    soonest_dts = 1;
    }
    
    // TODO(Val): check that packets are valid (or existed)
    
    if(soonest_dts != -1)
    {
    dbg_print("pkt[0].pts = %ld\n"
    "pkt[1].pts = %ld\n"
    "AV_NOPTS_VALUE = %ld\n",
    pkt[0]->pts,
    pkt[1]->pts,
    AV_NOPTS_VALUE);
    AVPacket* packet;
    avpacket_queue *queue = soonest_dts ? pdata->pq_video : pdata->pq_audio; 
    dequeue_packet(queue, &packet);
    
    dbg_print("dts: %ld\n", packet->dts);
    
    struct frame_info f = wt_decode(pdata, packet);
    
    if(f.type == FRAME_AUDIO &&
    !pdata->audio.is_ready)
    {
    process_audio_frame(pdata, f);
    }
    else if((f.type == FRAME_VIDEO_YUV ||
    f.type == FRAME_VIDEO_RGB) &&
    !pdata->video.is_ready)
    {
    process_video_frame(pdata, f);
    }
    else
    {
    dbg_error("Unknown frame type.\n");
    }
    
    av_frame_free(&f.frame);
    }
    
    PlatformSleep(33);
    */
    
    return 0;
}
