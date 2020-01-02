/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef VIDEO_H
#define VIDEO_H

#include <libavutil/rational.h>
#include "defines.h"
#include "avframe_pts_ordered_queue.h"

typedef struct _output_video {
    avframe_queue queue;
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
    bool32 is_ready;
    real64 pts;
} output_video;
 
static int32
ClearVideoOutput(output_video *video);

#endif
