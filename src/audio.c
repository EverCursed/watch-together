#include "audio.h"


static void
PrepareAudioOutput(output_audio *audio)
{
    free(audio->buffer);
    audio->buffer = NULL;
    audio->is_ready = 0;
    audio->duration = 0.0f;
    audio->size = 0;
}

