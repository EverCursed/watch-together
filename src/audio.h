#ifndef AUDIO_H
#define AUDIO_H

#include "defines.h"

typedef struct _output_audio {
    void *buffer;
    AVRational time_base;
    uint32 sample_rate;
    uint32 bytes_per_sample;
    uint32 channels;
    uint32 sample_format;
    uint32 size;
    volatile bool32 is_ready;
    real64 pts;
    real64 duration;
    real64 required_duration;
} output_audio;

void
PrepareAudioOutput(output_audio *);

#endif