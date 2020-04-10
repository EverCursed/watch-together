/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Audio module.
*/

#include "audio.h"

internal void
PrepareAudioOutput(output_audio *audio)
{
    audio->duration = 0.0;
    audio->size = 0;
}

internal r32
Volume(output_audio *audio)
{
    return audio->volume;
}

internal void
SetVolume(output_audio *audio, r32 v)
{
    audio->volume = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

internal void
IncreaseVolume(output_audio *audio, r32 v)
{
    r32 new_vol = audio->volume + v;
    
    SetVolume(audio, new_vol);
}
