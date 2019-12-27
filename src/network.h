#ifndef NETWORK_H
#define NETWORK_H

#include "communication_messages.h"
#include "defines.h"
#include "attributes.h"

typedef union _IPv4 {
    struct {
        uint8 a;
        uint8 b;
        uint8 c;
        uint8 d;
    } no_padding;
    struct {
        uint8 octet[4];
    };
    uint32 ip;
} IPv4;

typedef union _IPv6 {
    struct {
        uint16 block[8];
    };
    uint128 ip;
} IPv6;

typedef struct _destination_IP {
    union {
        struct {
            IPv4 v4;
            uint32 _pad[3];
        } no_padding;
        struct {
            IPv6 v6;
        };
    };
    bool32 is_ipv6;
} destination_IP;

not_used static void
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
    int32 type;
} net_message;

#define msg_header union { net_message msg; int32 type; }

struct align(4) _request_init_msg {
    msg_header;
};

struct align(4) _init_msg {
    msg_header;
    int32 flags;
    real64 start_time;
    real64 file_duration;
    uint16 port;
};

struct align(4) _finish_init_msg {
    msg_header;
    uint32 ip;
};

struct align(4) _ready_playback_msg {
    msg_header;
};

struct align(4) _request_port_msg {
    msg_header;
    uint32 ip;
};

struct align(4) _request_info_msg {
    msg_header;
};

struct align(4) _pause_msg {
    msg_header;
    real64 time;
};

struct align(4) _play_msg {
    msg_header;
};

struct align(4) _seek_msg {
    msg_header;
    real64 time;
};

struct align(4) _disconnect_msg {
    msg_header;
};


not_used static void
print_request_init_msg(struct _request_init_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_REQUEST_INIT\n",
             received ? "Receiving" : "Sending");
}

not_used static void
print_init_msg(struct _init_msg *msg, bool32 received)
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

not_used static void
print_ready_playback_msg(struct _ready_playback_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_READY_PLAYBACK\n",
             received ? "Receiving" : "Sending");
}

not_used static void
print_finish_init_msg(struct _finish_init_msg *msg, bool32 received)
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

not_used static void
print_play_msg(struct _play_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_PLAY\n",
             received ? "Receiving" : "Sending");
}

not_used static void
print_pause_msg(struct _pause_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_PAUSE\n",
             received ? "Receiving" : "Sending");
}

not_used static void
print_seek_msg(struct _seek_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_SEEK\n"
             "\ttime: %lf\n",
             received ? "Receiving" : "Sending",
             msg->time);
}

not_used static void
print_disconnect_msg(struct _disconnect_msg *msg, bool32 received)
{
    dbg_info("%s:\n"
             "\tMESSAGE_DISCONNECT\n",
             received ? "Receiving" : "Sending");
}

int32 StartServer();
int32 StartClient();
int32 CloseServer();
int32 AcceptConnection();
int32 ConnectToIP(const char *);
int32 SendControlMessages();
net_message *GetNextMessage();
int32 ReceiveControlMessages();
int32 SendInitRequestMessage();
int32 SendInitMessage(real64, real64, int32);
int32 SendFinishInitMessage(destination_IP);
int32 SendReadyPlaybackMessage();
int32 SendRequestPortMessage();
int32 SendPlayMessage();
int32 SendPauseMessage();
int32 SendSeekMessage(real64);
int32 SendDisconnectMessage();
int32 CloseConnection();
void GetPartnerIPStr(char **buffer);
void GetPartnerIPInt(uint32 *buffer);

#endif
