/*

This holds a queue of AVPacket structs. This is a buffer for 
     - 1) decoding for video playback
     - 2) transfering packets to peer
     
*/

#include <libavcodec/avcodec.h>
#include "packet_queue.h"
#include "watchtogether.h"

#define AVPACKET_SIZE sizeof(AVPacket)

static avpacket_queue*
init_avpacket_queue(int32 n)
{
    avpacket_queue *queue = malloc(sizeof(avpacket_queue));
    queue->buffer = malloc(AVPACKET_SIZE * n);
    
    queue->n = 0;
    queue->maxn = n;
    queue->next = 0;
    queue->end = 0;
    
    return queue;
}

static int32
enqueue_packet(avpacket_queue *queue, AVPacket *packet)
{
    if(queue->n == queue->maxn)
    {
        dbg_info("Packet queue full.\n");
        return -1;
    }
    
    av_packet_ref((queue->buffer + queue->end*AVPACKET_SIZE), packet);
    
    queue->n++;
    queue->end = (queue->end + 1) % queue->maxn;
    
    return 0;
}

static int32
dequeue_packet(avpacket_queue *queue, AVPacket *packet)
{
    if(queue->n == 0)
    {
        dbg_info("Packet queue empty.\n");
        return -1;
    }
    
    av_packet_ref(packet, (queue->buffer + queue->end*AVPACKET_SIZE));
    av_packet_unref((queue->buffer + queue->end*AVPACKET_SIZE));
    
    queue->n--;
    queue->next = (queue->next + 1) % queue->maxn;
    
    return 0;
}

// TODO(Val): Should we make another reference? Will need to unref manually then.
static int32
peek_packet(avpacket_queue *queue, AVPacket *packet, int nth)
{
    // check if nth packet is queued up
    if(!(queue->n > nth))
    {
        dbg_info("Peek packet: There is no packet in this location.");
        return -1;
    }
    
    int index_t = (queue->next + nth) % queue->maxn;
    
    av_packet_ref(packet, (queue->buffer + index_t*AVPACKET_SIZE));
    
    return 0;
}

static int32
clear_avpacket_queue(avpacket_queue *queue)
{
    for(int i = queue->next; i < queue->end; i = (i+1) % queue->maxn)
    {
        av_packet_unref(queue->buffer + i*AVPACKET_SIZE);
    }
    
    queue->n = 0;
    queue->next = 0;
    queue->end = 0;
    
    return 0;
}

static int32
close_avpacket_queue(avpacket_queue *queue)
{
    clear_avpacket_queue(queue);
    
    if(queue->buffer)
        free(queue->buffer);
    
    free(queue);
    
    return 0;
}

static inline bool32
pq_is_full(avpacket_queue *queue)
{
    return (queue->n == queue->maxn);
}

static inline bool32
pq_is_empty(avpacket_queue *queue)
{
    return (queue->n == 0);
}