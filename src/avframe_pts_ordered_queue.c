#include "avframe_pts_ordered_queue.h"
#include "platform.h"
#include "errors.h"

int32
avframe_queue_init(avframe_queue *queue, int32 flags)
{
    queue->mutex = PlatformCreateMutex();
    queue->max = MAX_FRAMES;
    queue->n = 0;
    queue->flags = flags;
    
    RETURN(SUCCESS);
}

int32
avframe_queue_deinit(avframe_queue *queue)
{
    PlatformDestroyMutex(&queue->mutex);
    queue->max = 0;
    queue->n = 0;
    
    RETURN(SUCCESS);
}

int32
avframe_queue_enqueue(avframe_queue *queue, AVFrame *frame, real64 pts)
{
    if(avframe_queue_initialized(queue))
        RETURN(NOT_INITIALIZED);
    if(avframe_queue_full(queue))
        RETURN(CONTAINER_FULL);
    
    PlatformLockMutex(&queue->mutex);
    
    real64 t;
    AVFrame *f;
    int i = queue->n;
    queue->frames[i] = frame;
    
    if(queue->flags & AVFQ_ORDERED_PTS)
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
    /*
    dbg_info("frame queued:\n"
             "\tformat:  %d\n"
             "\tpointer: %p\n"
             "\twidth:   %d\n"
             "\theight:  %d\n"
             "\tsamples: %d\n",
             frame->format,
             frame->data[0],
             frame->width,
             frame->height,
             frame->nb_samples);
    */
    queue->n++;
    
    PlatformUnlockMutex(&queue->mutex);
    
    RETURN(SUCCESS);
}

// TODO(Val): This should also check if we don't provide a pointer to pts
int32
avframe_queue_dequeue(avframe_queue *queue, AVFrame **frame, real64 *pts)
{
    if(avframe_queue_initialized(queue))
        RETURN(NOT_INITIALIZED);
    if(avframe_queue_empty(queue))
        RETURN(CONTAINER_EMPTY);
    
    PlatformLockMutex(&queue->mutex);
    
    int i;
    
    *frame = queue->frames[0];
    for(i = 0; i < queue->n-1; i++)
        queue->frames[i] = queue->frames[i+1];
    queue->frames[i] = NULL;
    
    if(queue->flags & AVFQ_ORDERED_PTS) // TODO(Val): Do this flag differently
    {
        if(pts != NULL)
            *pts = queue->pts[0];
        
        for(i = 0; i < queue->n-1; i++)
            queue->pts[i] = queue->pts[i+1];
        queue->pts[i] = 0.0;
    }
    PlatformUnlockMutex(&queue->mutex);
    
    /*
    dbg_info("frame dequeued:\n"
             "\tformat:  %d\n"
             "\tpointer: %p\n"
             "\twidth:   %d\n"
             "\theight:  %d\n"
             "\tsamples: %d\n",
             (temp_frame)->format,
             (temp_frame)->data[0],
             (temp_frame)->width,
             (temp_frame)->height,
             (temp_frame)->nb_samples);
    */
    
    queue->n--;
    
    RETURN(SUCCESS);
}

int32
avframe_queue_remove(avframe_queue *queue, real64 pts)
{
    RETURN(NOT_YET_IMPLEMENTED);
}

int32
avframe_queue_clear(avframe_queue *queue)
{
    RETURN(NOT_YET_IMPLEMENTED);
}
