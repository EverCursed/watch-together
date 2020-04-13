/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Audio module header.
*/

#ifndef AUDIO_H
#define AUDIO_H

#include "defines.h"
#include "avframe_pts_ordered_queue.h"

typedef struct _output_audio {
    avframe_queue *queue;
    platform_mutex mutex;
    
    AVRational time_base;
    u32 sample_rate;
    u32 bytes_per_sample;
    u32 channels;
    u32 sample_format;
    u32 size;
    u32 max_buffer_size;
    b32 is_ready;
    f64 pts;
    f64 duration;
    
    i32 audio_format;
    
    f32 volume;
} output_audio;


internal void PrepareAudioOutput(output_audio *);
internal f32 Volume(output_audio *audio);
internal void SetVolume(output_audio *audio, f32 v);
internal void IncreaseVolume(output_audio *audio, f32 v);

#endif
