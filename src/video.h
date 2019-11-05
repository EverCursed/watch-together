#ifndef VIDEO_H
#define VIDEO_H

#include <libavutil/rational.h>
#include "defines.h"

typedef struct _ouput_video {
    void *video_frame;
    void *video_frame_sup1;
    void *video_frame_sup2;
    real64 frame_duration;
    uint32 pitch;
    uint32 pitch_sup1;
    uint32 pitch_sup2;
    uint32 width;
    uint32 height;
    int32 type;
    volatile bool32 is_ready;
    real64 pts;
} output_video;

int32
ClearVideoOutput(output_video *video);

#endif
