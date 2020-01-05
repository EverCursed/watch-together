/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

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

internal avpacket_queue* init_avpacket_queue(int32 n);
internal int32 enqueue_packet(avpacket_queue *queue, AVPacket *packet);
internal int32 dequeue_packet(avpacket_queue *queue, AVPacket **packet);
internal int32 peek_packet(avpacket_queue *queue, AVPacket **packet, int nth);
internal int32 clear_avpacket_queue(avpacket_queue *queue);
internal int32 close_avpacket_queue(avpacket_queue **queue);

internal void print_packets(avpacket_queue *queue);

internal inline bool32 pq_is_full(avpacket_queue *queue)
{
    return (queue->n == queue->maxn);
}

internal inline bool32 pq_is_empty(avpacket_queue *queue)
{
    return (queue->n == 0);
}

internal inline b32 pq_initialized(avpacket_queue *queue)
{
    return (queue->maxn > 0);
}

#endif