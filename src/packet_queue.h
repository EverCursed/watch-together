#ifndef PACKET_QUEUE
#define PACKET_QUEUE

typedef struct avpacket_queue {
    int32 maxn;  // max number of packets
    int32 n;     // total number of packets
    int32 next;  // the packet that will be dequeued/peeked next
    int32 end;   // the where the next packet will be enqueued
    AVPacket *buffer;
} avpacket_queue;

#endif