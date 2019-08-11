#include "audio.h"


static void
PrepareAudioOutput(output_audio *audio)
{
    audio->duration = 0.0f;
    audio->size = 0;
    audio->is_ready = 0;
}

