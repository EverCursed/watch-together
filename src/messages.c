#include "defines.h"
#include "messages.h"

global message null_msg = {MSG_INVALID, 0};

global int queue_size = {0};
message queue[MESSAGE_QUEUE_SIZE] = {0};

internal void set_null(message* msg)
{
    msg->msg = 0;
    msg->additional = 0;
}

int message_enqueue(message msg)
{
    if(queue_size < MESSAGE_QUEUE_SIZE - 1)
    {
        queue[queue_size] = msg;
        queue_size++;
        return 1;
    }
    
    return 0;
}

message message_dequeue()
{
    if(queue_size)
    {
        message msg = queue[queue_size--];
        set_null(&queue[queue_size]);
        return msg;
    }
    return null_msg;
}

