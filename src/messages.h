#ifndef WT_MESSAGES_H
#define WT_MESSAGES_H

#define MESSAGE_QUEUE_SIZE    64

typedef struct message {
    int msg;
    int additional;
    // TODO(Val): Anything else?
} message;

typedef struct msg_node {
    message msg;
    message* prev;
    message* next;
} node;

typedef struct message_queue {
    message* head;
    message* tail;
} message_queue;

#endif 