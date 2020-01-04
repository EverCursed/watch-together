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
