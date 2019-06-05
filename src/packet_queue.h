#ifndef PACKET_QUEUE
#define PACKET_QUEUE

typedef struct avpacket_queue {
    int32 maxn;  // max number of packets
    int32 n;     // total number of packets
    int32 next;  // the packet that will be dequeued/peeked next
    int32 end;   // the where the next packet will be enqueued
    AVPacket *buffer;
} avpacket_queue;


static avpacket_queue* init_avpacket_queue(int32 n);
static int32 enqueue_packet(avpacket_queue *queue, AVPacket packet);
static int32 dequeue_packet(avpacket_queue *queue, AVPacket *packet);
static int32 peek_packet(avpacket_queue *queue, AVPacket *packet, int nth);
static int32 clear_avpacket_queue(avpacket_queue *queue);
static int32 close_avpacket_queue(avpacket_queue *queue);

#endif