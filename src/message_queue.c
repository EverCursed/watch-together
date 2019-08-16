#include "message_queue.h"

// TODO(Val): Add way to close queue? Although I don't know when we would want to stop getting input

static void
InitMessageQueue(message_queue *q)
{
    q->start = 0;
    q->end = 0;
    q->n = 0;
    q->max = MSG_QUEUE_SIZE;
}

static void
AddMessage(message_queue *q, int32 m, arg a1, arg a2, arg a3, arg a4, arg a5, real64 time)
{
    if(q->n < q->max)
    {
        message *message = &q->queue[q->end];
        
        message->msg = m;
        
        message->arg1 = a1;
        message->arg2 = a2;
        message->arg3 = a3;
        message->arg4 = a4;
        message->arg5 = a5;
        
        message->time = time;
        
        q->end = (q->end + 1) % q->max;
        q->n++;
    }
}

static int32
GetMessage(message_queue *q, message *m)
{
    if(q->n > 0)
    {
        *m = q->queue[q->start];
        
        q->start = (q->start + 1) % q->max;
        q->n--;
        
        return 0;
    }
    else
    {
        m->msg = MSG_NO_MORE_MESSAGES;
        dbg_error("There were no more messages.\n");
        return -1;
    }
}

static void
ClearMessages(message_queue *q)
{
    q->n = 0;
    q->start = 0;
    q->end = 0;
}

static bool32
MessagesEmpty(message_queue *q)
{
    return (q->n == 0);
}

static bool32
MessagesFull(message_queue *q)
{
    return (q->n == q->max);
}