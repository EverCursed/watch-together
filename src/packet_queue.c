/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

/*

This holds a queue of AVPacket structs. This is a buffer for 
     - 1) decoding for video playback
     - 2) transfering packets to peer
     
*/

#include <libavcodec/avcodec.h>
#include "common/custom_malloc.h"

#include "watchtogether.h"
#include "defines.h"

internal avpacket_queue*
PQInit(int32 n)
{
    avpacket_queue *queue;
    queue = custom_malloc(sizeof(avpacket_queue));
    if(!queue) return NULL;
    
    queue->array = custom_malloc(n * sizeof(AVPacket *));
    if(!queue->array) {
        custom_free(queue);
        return NULL;
    }
    
    queue->n = 0;
    queue->maxn = n;
    queue->next = 0;
    queue->end = 0;
    
    queue->mutex = PlatformCreateMutex();
    
    return queue;
}

internal int32
PQEnqueue(avpacket_queue *queue, AVPacket *packet)
{
    if(!PQInitialized(queue))
    {
        RETURN(UNINITIALIZED);
    }
    
    if(queue->n == queue->maxn)
    {
        dbg_warn("Packet queue full.\n");
        RETURN(CONTAINER_FULL);
    }
    //dbg_info("Enqueueing packet.\n");
    //dbg_packet(packet);
    if(success(PlatformLockMutex(&queue->mutex)))
    {
        queue->array[queue->end] = packet;
        queue->n++;
        queue->end = (queue->end + 1) % queue->maxn;
        
        PlatformUnlockMutex(&queue->mutex);
        RETURN(SUCCESS);
    }
    else
    {
        RETURN(UNKNOWN_ERROR);
    }
}

internal int32
PQDequeue(avpacket_queue *queue, AVPacket **packet)
{
    if(!PQInitialized(queue))
    {
        RETURN(UNINITIALIZED);
    }
    
    if(queue->n == 0)
    {
        dbg_warn("Packet queue empty.\n");
        RETURN(NOT_ENOUGH_DATA);
    }
    
    if(success(PlatformLockMutex(&queue->mutex)))
    {
        *packet = queue->array[queue->next];
        queue->n--;
        queue->next = (queue->next + 1) % queue->maxn;
        
        PlatformUnlockMutex(&queue->mutex);
        RETURN(SUCCESS);
    }
    else
    {
        dbg_error("Couldn't lock mutex. dequeue_packed() failed.\n");
        RETURN(UNKNOWN_ERROR);
    }
}

internal int32
PQPeek(avpacket_queue *queue, AVPacket **packet, int nth)
{
    //check if nth packet is queued up
    if(!PQInitialized(queue))
    {
        RETURN(UNINITIALIZED);
    }
    
    if(queue->n < nth+1)
    {
        dbg_print("packet queue: %d\n", queue->n);
        RETURN(WRONG_ARGS);
    }
    
    int index_t = (queue->next + nth) % queue->maxn;
    
    *packet = queue->array[index_t];
    
    RETURN(SUCCESS);
}

internal int32
PQClear(avpacket_queue *queue)
{
    PlatformLockMutex(&queue->mutex);
    
    queue->n = 0;
    queue->next = 0;
    queue->end = 0;
    
    PlatformUnlockMutex(&queue->mutex);
    
    RETURN(SUCCESS);
}

internal int32
PQClose(avpacket_queue **queue)
{
    // TODO(Val): Should this also free all the packets queued up?
    
    avpacket_queue *q = *queue;
    
    q->maxn = 0;
    PQClear(*queue); 
    PlatformDestroyMutex(&q->mutex);
    
    if(q->array)
        custom_free(q->array);
    
    custom_free(q);
    *queue = NULL;
    
    RETURN(SUCCESS);
}

internal void
print_packets(avpacket_queue *queue)
{
    int32 i = queue->next;
    while(i != queue->end)
    {
        AVPacket *packet = queue->array[i];
        
        dbg_info("Packet %d: pts - %ld\n"
                 "           dts - %ld\n",
                 i,
                 (long int)packet->pts,
                 (long int)packet->dts);
        
        i = (i + 1) % queue->maxn;
    }
}
