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

#define FQ_MAX_FRAMES 30 

#define FQ_ORDERED_ENTRY 0x1
#define FQ_ORDERED_PTS   0x2

#define FQ_NO_MORE_FRAMES -10000000.0f

typedef struct _avframe_queue {
    AVFrame **frames;
    f64 *pts;
    platform_mutex mutex;
    i32 max;
    i32 n;
    i32 flags;
} avframe_queue;

warn_unused internal inline b32
FQFull(avframe_queue *queue) 
{
    return (queue->n == queue->max);
}

warn_unused internal inline b32
FQEmpty(avframe_queue *queue)
{
    return (queue->n == 0);
}

warn_unused internal inline f64
FQNextTimestamp(avframe_queue *queue)
{
    return queue->n > 0 ? queue->pts[0] : FQ_NO_MORE_FRAMES;
}

warn_unused internal inline b32
FQInitialized(avframe_queue *queue)
{
    return (queue->max == 0);
}

internal i32
FQInit(avframe_queue *queue, i32 flags);

internal void
FQClose(avframe_queue **queue);

warn_unused internal i32
FQDequeue(avframe_queue *queue, AVFrame **frame, f64 *pts);

warn_unused internal i32
FQEnqueue(avframe_queue *queue, AVFrame *frame, f64 pts);

warn_unused internal i32
FQRemoveUpTo(avframe_queue *queue, f64 pts);

warn_unused internal i32
FQClear(avframe_queue *queue);

#endif // AVFRAME_QUEUE_PTS_ORDERED
