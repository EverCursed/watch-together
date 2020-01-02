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
    avframe_queue queue;
    void *buffer;
    platform_mutex mutex;
    AVRational time_base;
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
    uint32 size;
    uint32 max_buffer_size;
    bool32 is_ready;
    real64 pts;
    real64 duration;
} output_audio;

static void PrepareAudioOutput(output_audio *);

#endif