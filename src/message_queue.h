#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include "defines.h"

#define MSG_QUEUE_SIZE 16

#define MSG_NO_MORE_MESSAGES   0
#define MSG_START_PLAYBACK     1
#define MSG_STOP_PLAYBACK      2
#define MSG_PAUSE              3
#define MSG_SEEK               4
#define MSG_CONNECT            5
#define MSG_WINDOW_RESIZED     6
#define MSG_CLOSE              7
#define MSG_TOGGLE_FULLSCREEN  8

typedef struct _arg {
    union {
        int32 s;
        uint32 u;
        real32 f;
    };
} arg;

arg NO_ARG = { .s = 0 };

typedef struct _message {
    int32 msg;
    
    arg arg1;
    arg arg2;
    arg arg3;
    arg arg4;
    arg arg5;
    
    real64 time;
} message;

typedef struct _message_queue {
    message queue[MSG_QUEUE_SIZE];
    
    int32 start;
    int32 end;
    int32 n;
    int32 max;
} message_queue;


static void InitMessageQueue(message_queue *q);
static void AddMessage(message_queue *q, int32 m, arg a1, arg a2, arg a3, arg a4, arg a5, real64 time);
static int32 GetMessage(message_queue *q, message *m);
static void ClearMessages(message_queue *q);
static bool32 MessagesEmpty(message_queue *q);
static bool32 MessagesFull(message_queue *q);

#endif