#ifndef PACKET_QUEUE
#define PACKET_QUEUE

#include "defines.h"

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

typedef struct _avpacket_queue {
    //AVPacket *buffer;
    AVPacket **array;
    platform_mutex mutex;
    int32 maxn;  // max number of packets
    int32 n;     // total number of packets
    int32 next;  // the packet that will be dequeued/peeked next
    int32 end;   // the where the next packet will be enqueued
} avpacket_queue;

avpacket_queue* init_avpacket_queue(int32 n);
int32 enqueue_packet(avpacket_queue *queue, AVPacket *packet);
int32 dequeue_packet(avpacket_queue *queue, AVPacket **packet);
int32 peek_packet(avpacket_queue *queue, AVPacket **packet, int nth);
int32 clear_avpacket_queue(avpacket_queue *queue);
int32 close_avpacket_queue(avpacket_queue *queue);

void print_packets(avpacket_queue *queue);

static inline bool32 pq_is_full(avpacket_queue *queue)
{
    return (queue->n == queue->maxn);
}

static inline bool32 pq_is_empty(avpacket_queue *queue)
{
    return (queue->n == 0);
}

#endif