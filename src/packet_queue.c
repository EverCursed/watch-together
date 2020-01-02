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

avpacket_queue*
init_avpacket_queue(int32 n)
{
    avpacket_queue *queue = custom_malloc(sizeof(avpacket_queue));
    queue->array = custom_malloc(sizeof(AVPacket *)*n);
    
    queue->n = 0;
    queue->maxn = n;
    queue->next = 0;
    queue->end = 0;
    
    queue->mutex = PlatformCreateMutex();
    
    return queue;
}

int32
enqueue_packet(avpacket_queue *queue, AVPacket *packet)
{
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

int32
dequeue_packet(avpacket_queue *queue, AVPacket **packet)
{
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
        
        //dbg_info("Dequeueing packet.\n");
        //dbg_packet((*packet));
        
        PlatformUnlockMutex(&queue->mutex);
        RETURN(SUCCESS);
    }
    else
    {
        dbg_error("Couldn't lock mutex. dequeue_packed() failed.\n");
        RETURN(UNKNOWN_ERROR);
    }
}

int32
peek_packet(avpacket_queue *queue, AVPacket **packet, int nth)
{
    // check if nth packet is queued up
    if(queue->n < nth+1)
    {
        dbg_print("packet queue: %d\n", queue->n);
        RETURN(WRONG_ARGS);
    }
    
    int index_t = (queue->next + nth) % queue->maxn;
    
    *packet = queue->array[index_t];
    
    RETURN(SUCCESS);
}

int32
clear_avpacket_queue(avpacket_queue *queue)
{
    PlatformLockMutex(&queue->mutex);
    
    queue->n = 0;
    queue->next = 0;
    queue->end = 0;
    
    RETURN(SUCCESS);
}

int32
close_avpacket_queue(avpacket_queue **queue)
{
    clear_avpacket_queue(*queue);
    
    if((*queue)->array)
        free((*queue)->array);
    
    free(*queue);
    *queue = NULL;
    
    RETURN(SUCCESS);
}

void
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
