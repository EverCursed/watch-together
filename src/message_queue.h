/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include "defines.h"

#define MSG_QUEUE_SIZE 16

#define MSG_NO_MORE_MESSAGES    0
#define MSG_START_PLAYBACK      1
#define MSG_STOP_PLAYBACK       2
#define MSG_PAUSE               3
#define MSG_SEEK                4
#define MSG_CONNECT             5
#define MSG_WINDOW_RESIZED      6
#define MSG_CLOSE               7
#define MSG_TOGGLE_FULLSCREEN   8
#define MSG_VOLUME_CHANGE       9
#define MSG_FILE_OPEN          10
#define MSG_FILE_CLOSE         11
#define MSG_START_SERVER       12
#define MSG_CONNECT_TO_SERVER  13
#define MSG_DISCONNECT         14
#define MSG_START_CLIENT       15
#define MSG_TOGGLE_MENU        16

typedef struct _arg {
    union {
        int32 s;
        uint32 u;
        real32 f;
    };
} arg;

#pragma GCC diagnostic ignored "-Wunused-variable"
static arg NO_ARG = { .s = 0 };

typedef struct _message {
    int32 msg;
    arg args[5];
    real64 time;
} message;

typedef struct _message_queue {
    message queue[MSG_QUEUE_SIZE];
    
    int32 start;
    int32 end;
    int32 n;
    int32 max;
} message_queue;

#define AddMessage0(q,m,t) AddMessage(q, m, NO_ARG, NO_ARG, NO_ARG, NO_ARG, NO_ARG, t)
#define AddMessage1(q,m,a,t) AddMessage(q, m, a, NO_ARG, NO_ARG, NO_ARG, NO_ARG, t)
#define AddMessage2(q,m,a,b,t) AddMessage(q, m, a, b, NO_ARG, NO_ARG, NO_ARG, t)
#define AddMessage3(q,m,a,b,c,t) AddMessage(q, m, a, b, c, NO_ARG, NO_ARG, t)
#define AddMessage4(q,m,a,b,c,d,t) AddMessage(q, m, a, b, c, d, NO_ARG, t)
#define AddMessage5(q,m,a,b,c,d,e,t) AddMessage(q, m, a, b, c, d, e, t)

internal inline bool32
MessagesEmpty(message_queue *q)
{
    return (q->n == 0);
}

internal inline bool32
MessagesFull(message_queue *q)
{
    return (q->n >= q->max);
}

internal void InitMessageQueue(message_queue *q);
internal void AddMessage(message_queue *q, int32 m, arg a1, arg a2, arg a3, arg a4, arg a5, real64 time);
internal int32 GetApplicationMessage(message_queue *q, message *m);
internal void ClearMessages(message_queue *q);

#endif