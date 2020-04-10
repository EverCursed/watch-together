/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

This is a queue that stores AVFrames by their timestamp. 
*/

#include "avframe_pts_ordered_queue.h"
#include "platform.h"
#include "errors.h"

internal avframe_queue *
FQInitialize(int32 flags)
{
    avframe_queue *queue = custom_malloc(sizeof(avframe_queue));
    if(!queue) goto alloc_fail;
    
    queue->frames = custom_malloc(FQ_MAX_FRAMES * sizeof(AVFrame *));
    if(!queue->frames) goto alloc_fail_inner;
    
    queue->pts = custom_malloc(FQ_MAX_FRAMES * sizeof(real64));
    if(!queue->pts) goto alloc_fail_inner;
    
    queue->mutex = PlatformCreateMutex();
    queue->max = FQ_MAX_FRAMES;
    queue->n = 0;
    queue->flags = flags;
    
    return queue;
    
    
    
    
    alloc_fail_inner:
    
    custom_free(queue->pts);
    custom_free(queue->frames);
    
    alloc_fail:
    
    custom_free(queue);
    
    return NULL;
}

internal void
FQClose(avframe_queue **queue)
{
    PlatformDestroyMutex(&(*queue)->mutex);
    
    custom_free((*queue)->pts);
    custom_free((*queue)->frames);
    custom_free(*queue);
    
    *queue = NULL;
}

internal int32
FQEnqueue(avframe_queue *queue, AVFrame *frame, real64 pts)
{
    if(FQInitialized(queue))
        RETURN(NOT_INITIALIZED);
    if(FQFull(queue))
        RETURN(CONTAINER_FULL);
    
    PlatformLockMutex(&queue->mutex);
    
    real64 t;
    AVFrame *f;
    int i = queue->n;
    queue->frames[i] = frame;
    
    if(queue->flags & FQ_ORDERED_PTS)
    {
        queue->pts[i] = pts;
        
        while(i > 0 && queue->pts[i] < queue->pts[i-1])
        {
            //swap
            t = queue->pts[i]; queue->pts[i] = queue->pts[i-1]; queue->pts[i-1] = t;
            f = queue->frames[i]; queue->frames[i] = queue->frames[i-1]; queue->frames[i-1] = f;
            
            i--;
        }
    }
    
    queue->n++;
    
    PlatformUnlockMutex(&queue->mutex);
    
    RETURN(SUCCESS);
}

// TODO(Val): This should also check if we don't provide a pointer to pts
internal int32
FQDequeue(avframe_queue *queue, AVFrame **frame, real64 *pts)
{
    if(FQInitialized(queue))
        RETURN(NOT_INITIALIZED);
    if(FQEmpty(queue))
        RETURN(CONTAINER_EMPTY);
    
    PlatformLockMutex(&queue->mutex);
    
    int i;
    
    *frame = queue->frames[0];
    for(i = 0; i < queue->n-1; i++)
        queue->frames[i] = queue->frames[i+1];
    queue->frames[i] = NULL;
    
    if(queue->flags & FQ_ORDERED_PTS) // TODO(Val): Do this flag differently
    {
        if(pts != NULL)
            *pts = queue->pts[0];
        
        for(i = 0; i < queue->n-1; i++)
            queue->pts[i] = queue->pts[i+1];
        queue->pts[i] = 0.0;
    }
    PlatformUnlockMutex(&queue->mutex);
    
    queue->n--;
    
    RETURN(SUCCESS);
}

internal int32
FQRemove(avframe_queue *queue, real64 pts)
{
    RETURN(NOT_YET_IMPLEMENTED);
}

internal int32
FQClear(avframe_queue *queue)
{
    RETURN(NOT_YET_IMPLEMENTED);
}
