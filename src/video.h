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
    avframe_queue *queue;
    void *video_frame;
    void *video_frame_sup1;
    void *video_frame_sup2;
    f64 frame_duration;
    u32 pitch;
    u32 pitch_sup1;
    u32 pitch_sup2;
    u32 width;
    u32 height;
    i32 type;
    b32 is_ready;
    f64 pts;
} output_video;

internal i32
ClearVideoOutput(output_video *video);

#endif
