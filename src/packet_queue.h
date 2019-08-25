#ifndef PACKET_QUEUE
#define PACKET_QUEUE

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

typedef struct _avpacket_queue avpacket_queue;

avpacket_queue* init_avpacket_queue(int32 n);
int32 enqueue_packet(avpacket_queue *queue, AVPacket *packet);
int32 dequeue_packet(avpacket_queue *queue, AVPacket **packet);
int32 peek_packet(avpacket_queue *queue, AVPacket **packet, int nth);
int32 clear_avpacket_queue(avpacket_queue *queue);
int32 close_avpacket_queue(avpacket_queue *queue);

bool32 pq_is_full(avpacket_queue *queue);
bool32 pq_is_empty(avpacket_queue *queue);

#endif