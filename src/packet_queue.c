/*

This holds a queue of AVPacket structs. This is a buffer for 
     - 1) decoding for video playback
     - 2) transfering packets to peer
     
*/

#include <libavcodec/avcodec.h>
#include "packet_queue.h"
#include "watchtogether.h"

// TODO(Val): Think about how the packets should be free'd.

#define dbg_packet(p) \
do { \
    dbg_info("Packet:\n" \
    "\tpos:\t%ld\n" \
    "\tindex:\t%d\n" \
    "\tpts:\t%ld\n", \
    p->pos, \
    p->stream_index, \
    p->pts); \
} while(0)

static avpacket_queue*
init_avpacket_queue(int32 n)
{
    avpacket_queue *queue = malloc(sizeof(avpacket_queue));
    queue->array = malloc(sizeof(AVPacket *)*n);
    
    queue->n = 0;
    queue->maxn = n;
    queue->next = 0;
    queue->end = 0;
    
    queue->mutex = SDL_CreateMutex();
    
    return queue;
}

static int32
enqueue_packet(avpacket_queue *queue, AVPacket *packet)
{
    if(queue->n == queue->maxn)
    {
        dbg_warn("Packet queue full.\n");
        return -1;
    }
    dbg_info("Enqueueing packet.\n");
    dbg_packet(packet);
    
    queue->array[queue->end] = packet;
    queue->n++;
    queue->end = (queue->end + 1) % queue->maxn;
    
    return 0;
}

static int32
dequeue_packet(avpacket_queue *queue, AVPacket **packet)
{
    if(queue->n == 0)
    {
        dbg_warn("Packet queue empty.\n");
        return -1;
    }
    
    if(!SDL_LockMutex(queue->mutex))
    {
        *packet = queue->array[queue->next];
        queue->n--;
        queue->next = (queue->next + 1) % queue->maxn;
        
        dbg_info("Dequeueing packet.\n");
        dbg_packet((*packet));
        
        SDL_UnlockMutex(queue->mutex);
        return 0;
    }
    else
    {
        return -1;
    }
}

// TODO(Val): Should we make another reference? Will need to unref manually then.
static int32
peek_packet(avpacket_queue *queue, AVPacket **packet, int nth)
{
    // check if nth packet is queued up
    if(!(queue->n > nth))
    {
        dbg_info("Peek packet: There is no packet in this location.\n");
        return -1;
    }
    
    int index_t = (queue->next + nth) % queue->maxn;
    
    *packet = queue->array[index_t];
    
    return 0;
}

static int32
clear_avpacket_queue(avpacket_queue *queue)
{
    /*
    for(int i = queue->next; i < queue->end; i = (i+1) % queue->maxn)
    {
        av_packet_unref(queue->buffer + i*AVPACKET_SIZE);
    }
    */
    
    queue->n = 0;
    queue->next = 0;
    queue->end = 0;
    
    return 0;
}

// TODO(Val): Make this take a reference to a pointer and set it to NULL afterwards
static int32
close_avpacket_queue(avpacket_queue *queue)
{
    clear_avpacket_queue(queue);
    
    if(queue->array)
        free(queue->array);
    //free(queue->buffer);
    
    free(queue);
    
    return 0;
}

static void
print_packets(avpacket_queue *queue, program_data *pdata)
{
    int32 i = queue->next;
    while(i != queue->end)
    {
        AVPacket *packet = queue->array[i];
        
        dbg_packet(packet);
        dbg_info("-----------------------------\n");
        
        i = (i + 1) % queue->maxn;
    }
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