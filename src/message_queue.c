/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#include "message_queue.h"

// TODO(Val): Add way to close queue? Although I don't know when we would want to stop getting input

internal void
InitMessageQueue(message_queue *q)
{
    q->start = 0;
    q->end = 0;
    q->n = 0;
    q->max = MSG_QUEUE_SIZE;
}

internal void
AddMessage(message_queue *q, i32 m, arg a1, arg a2, arg a3, arg a4, arg a5, f64 time)
{
    if(!MessagesFull(q))
    {
        message *message = &q->queue[q->end];
        
        message->msg = m;
        
        message->args[0] = a1;
        message->args[1] = a2; 
        message->args[2] = a3; 
        message->args[3] = a4; 
        message->args[4] = a5;
        
        message->time = time;
        
        q->end = (q->end + 1) % q->max;
        q->n++;
    }
}

internal i32
GetApplicationMessage(message_queue *q, message *m)
{
    if(q->n > 0)
    {
        *m = q->queue[q->start];
        
        q->start = (q->start + 1) % q->max;
        q->n--;
        
        RETURN(SUCCESS);
    }
    else
    {
        m->msg = MSG_NO_MORE_MESSAGES;
        dbg_error("There were no more messages.\n");
        
        RETURN(NOT_ENOUGH_DATA);
    }
}

internal void
ClearMessages(message_queue *q)
{
    q->n = 0;
    q->start = 0;
    q->end = 0;
}
