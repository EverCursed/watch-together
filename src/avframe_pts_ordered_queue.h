/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Timestamp queue header.
*/

#ifndef AVFRAME_QUEUE_PTS_ORDERED
#define AVFRAME_QUEUE_PTS_ORDERED

#include <libavutil/frame.h>
#include "defines.h"
#include "attributes.h"
#include "platform.h"

#define MAX_FRAMES 30 

#define AVFQ_ORDERED_ENTRY 0x1
#define AVFQ_ORDERED_PTS   0x2

typedef struct _avframe_queue {
    AVFrame *frames[MAX_FRAMES];
    real64 pts[MAX_FRAMES];
    platform_mutex mutex;
    int32 max;
    int32 n;
    int32 flags;
} avframe_queue;

warn_unused internal inline bool32
avframe_queue_full(avframe_queue *queue) 
{
    return (queue->n == queue->max);
}

warn_unused internal inline bool32
avframe_queue_empty(avframe_queue *queue)
{
    return (queue->n == 0);
}

warn_unused internal inline real64
avframe_queue_next_pts(avframe_queue *queue)
{
    return queue->pts[0];
}

warn_unused internal inline bool32
avframe_queue_initialized(avframe_queue *queue)
{
    return (queue->max == 0);
}

internal int32
avframe_queue_init(avframe_queue *queue, int32 flags);

internal int32
avframe_queue_deinit(avframe_queue *queue);

warn_unused internal int32
avframe_queue_dequeue(avframe_queue *queue, AVFrame **frame, real64 *pts);

warn_unused internal int32
avframe_queue_enqueue(avframe_queue *queue, AVFrame *frame, real64 pts);

warn_unused internal int32
avframe_queue_remove(avframe_queue *queue, real64 pts);

warn_unused internal int32
avframe_queue_clear(avframe_queue *queue);

#endif // AVFRAME_QUEUE_PTS_ORDERED
