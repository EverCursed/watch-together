#include "deb-watchtogether-v2.h"
#include "decoding.c"

// TODO(Val): ffmpeg decoding/encoding
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

static void
Processing(program_data *data)
{
    // TODO(Val): Process input
    video_get_next_frame(&data->Pixels);
    // TODO(Val): Get audio
    PlatformEnqueueAudio(data->SoundSample);
    // TODO(Val): Draw UI
    
    
}