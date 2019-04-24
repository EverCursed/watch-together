#include "video.c"
#include "deb-watchtogether-v2.h"

// TODO(Val): ffmpeg decoding/encoding
// TODO(Val): NAT-T implementation, see how it works
// TODO(Val): Encryption

static void
Processing(program_data *data)
{
    video_get_next_frame(&data->Pixels);
    //     draw video frame
    //     draw UI
    
    // Audio
    PlatformEnqueueAudio(data->SoundSample);
    
    //
}