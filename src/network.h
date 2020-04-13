/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef NETWORK_H
#define NETWORK_H

#include "communication_messages.h"
#include "defines.h"
#include "attributes.h"

#define MESSAGE_DATA_ALIGNMENT 8

typedef union _IPv4 {
    struct {
        u8 a;
        u8 b;
        u8 c;
        u8 d;
    } no_padding;
    struct {
        u8 octet[4];
    };
    u32 ip;
} IPv4;

typedef union _IPv6 {
    struct {
        u16 block[8];
    };
    uint128 ip;
} IPv6;

typedef struct _destination_IP {
    union {
        struct {
            IPv4 v4;
            u32 _pad[3];
        } no_padding;
        struct {
            IPv6 v6;
        };
    };
    b32 is_ipv6;
} destination_IP;

not_used internal void
IPToStr(char* buffer, destination_IP ip)
{
    if(ip.is_ipv6)
    {
        // TODO(Val): Write a IPv6 string conversion function
        dbg_error("Why is this marked?\n");
        buffer[0] = '\0';
    }
    else
    {
        sprintf(buffer, "%u.%u.%u.%u",
                ip.v4.a,
                ip.v4.b,
                ip.v4.c,
                ip.v4.d);
    }
}

// ------------------- messages ----------------------

typedef struct _message_type {
    i32 type;
} net_message;

#define MSG_HEADER \
union { \
    net_message msg; \
    i32 type; \
}

struct align(MESSAGE_DATA_ALIGNMENT) _request_init_msg {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _init_msg {
    MSG_HEADER;
    i32 flags;
    f64 start_time;
    f64 file_duration;
    u16 port;
};

struct align(MESSAGE_DATA_ALIGNMENT) _finish_init_msg {
    MSG_HEADER;
    u32 ip;
};

struct align(MESSAGE_DATA_ALIGNMENT) _ready_playback_msg {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _request_port_msg {
    MSG_HEADER;
    u32 ip;
};

struct align(MESSAGE_DATA_ALIGNMENT) _request_info_msg {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _pause_msg {
    MSG_HEADER;
    f64 time;
};

struct align(MESSAGE_DATA_ALIGNMENT) _play_msg {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _seek_msg {
    MSG_HEADER;
    f64 time;
};

struct align(MESSAGE_DATA_ALIGNMENT) _disconnect_msg {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _probe_rtt {
    MSG_HEADER;
};

struct align(MESSAGE_DATA_ALIGNMENT) _reply_rtt {
    MSG_HEADER;
};

#undef MSG_HEADER

not_used internal void
print_request_init_msg(struct _request_init_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_REQUEST_INIT\n",
             received ? "Receiving" : "Sending");
}

not_used internal void
print_init_msg(struct _init_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_INIT\n"
             "\tflags: %d\n"
             "\tstart_time: %lf\n"
             "\tfile_duration: %lf\n",
             received ? "Receiving" : "Sending",
             msg->flags,
             msg->start_time,
             msg->file_duration);
}

not_used internal void
print_ready_playback_msg(struct _ready_playback_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_READY_PLAYBACK\n",
             received ? "Receiving" : "Sending");
}

not_used internal void
print_finish_init_msg(struct _finish_init_msg *msg, b32 received)
{
    destination_IP ip = {};
    ip.v4.ip = msg->ip;
    
    dbg_error("IP_ADDRESS: %u\n", msg->ip);
    
    char ip_buf[16];
    IPToStr(ip_buf, ip);
    
    dbg_info("%s:\n"
             "\tMESSAGE_FINISH_INIT\n"
             "\tip: %s\n",
             received ? "Receiving" : "Sending",
             ip_buf);
}

not_used internal void
print_play_msg(struct _play_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_PLAY\n",
             received ? "Receiving" : "Sending");
}

not_used internal void
print_pause_msg(struct _pause_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_PAUSE\n",
             received ? "Receiving" : "Sending");
}

not_used internal void
print_seek_msg(struct _seek_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_SEEK\n"
             "\ttime: %lf\n",
             received ? "Receiving" : "Sending",
             msg->time);
}

not_used internal void
print_disconnect_msg(struct _disconnect_msg *msg, b32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_DISCONNECT\n",
             received ? "Receiving" : "Sending");
}

internal i32 StartServer();
internal i32 StartClient();
internal i32 CloseServer();
internal i32 AcceptConnection();
internal i32 ConnectToIP(const char *);
internal i32 SendControlMessages();
internal net_message *GetNextMessage();
internal i32 ReceiveControlMessages();
internal i32 SendInitRequestMessage();
internal i32 SendInitMessage(f64, f64, i32);
internal i32 SendFinishInitMessage(destination_IP);
internal i32 SendReadyPlaybackMessage();
internal i32 SendRequestPortMessage();
internal i32 SendPlayMessage();
internal i32 SendPauseMessage();
internal i32 SendSeekMessage(f64);
internal i32 SendDisconnectMessage();
internal i32 CloseConnection();
internal void GetPartnerIPStr(char **buffer);
internal u32  GetPartnerIPInt();

#endif
