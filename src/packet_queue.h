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
    i32 maxn;  // max number of packets
    i32 n;     // total number of packets
    i32 next;  // the packet that will be dequeued/peeked next
    i32 end;   // the where the next packet will be enqueued
} avpacket_queue;

internal avpacket_queue* PQInit(i32 n);
internal i32 PQEnqueue(avpacket_queue *queue, AVPacket *packet);
internal i32 PQDequeue(avpacket_queue *queue, AVPacket **packet);
internal i32 PQPeek(avpacket_queue *queue, AVPacket **packet, int nth);
internal i32 PQClear(avpacket_queue *queue);
internal i32 PQClose(avpacket_queue **queue);

internal void print_packets(avpacket_queue *queue);

internal inline b32 PQFull(avpacket_queue *queue)
{
    return (queue->n == queue->maxn);
}

internal inline b32 PQEmpty(avpacket_queue *queue)
{
    return (queue->n == 0);
}

internal inline b32 PQInitialized(avpacket_queue *queue)
{
    return (queue->maxn > 0);
}

#endif