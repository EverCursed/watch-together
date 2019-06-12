#ifndef PACKET_QUEUE
#define PACKET_QUEUE

typedef struct _avpacket_queue avpacket_queue;

static avpacket_queue* init_avpacket_queue(int32 n);
static int32 enqueue_packet(avpacket_queue *queue, AVPacket packet);
static int32 dequeue_packet(avpacket_queue *queue, AVPacket *packet);
static int32 peek_packet(avpacket_queue *queue, AVPacket *packet, int nth);
static int32 clear_avpacket_queue(avpacket_queue *queue);
static int32 close_avpacket_queue(avpacket_queue *queue);

#endif