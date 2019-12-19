#include <SDL2/SDL_net.h>

#include "network.h"
#include "utils.h"
#include "communication_messages.h"
#include "common/custom_malloc.h"

static uint16 message_ports[4] = {8212, 8418, 23458, 28002};
//static uint16 rtp_ports[4] = {9165, 9347, 18669, 21222};

/*

TCPsocket SDLNet_TCP_Open(IPaddress *ip)
void SDLNet_TCP_Close(TCPsocket sock)
TCPsocket SDLNet_TCP_Accept(TCPsocket server)
IPaddress *SDLNet_TCP_GetPeerAddress(TCPsocket sock)
int SDLNet_TCP_Send(TCPsocket sock, const void *data, int len)
int SDLNet_TCP_Recv(TCPsocket sock, void *data, int maxlen)

*/

#define MAX_NETWORK_BUFFER_SIZE 2048

static SDLNet_SocketSet socket_set;

static bool32 is_host = 0;
static bool32 is_client = 0;

static TCPsocket server = NULL;
static TCPsocket partner = NULL;

static void *temp_buffer = NULL;
//static int8 temp_buffer[MAX_NETWORK_BUFFER_SIZE] = {};
static int32 temp_buffer_used = 0;

static void *recv_buffer = NULL;
//static int8 recv_buffer[MAX_NETWORK_BUFFER_SIZE] = {};
static int32 recv_buffer_used = 0;
static int32 recv_buffer_size = 0;

#define Qmsg_init(msg_type, struct_type, var) \
struct_type var; \
var.type = msg_type; \
do { } while(0)

#define Qmsg_send(var) \
if(QueueControlMessage(&var, sizeof(var))) \
RETURN(UNKNOWN_ERROR); \
else \
RETURN(SUCCESS); \
do { } while(0)

//*(type *)ptr = *(type *)src; 

#define copy_msg(type, ptr, src) \
do { \
    ptr = custom_malloc(sizeof(type)); \
    memcpy(ptr, src, sizeof(type)); \
    recv_buffer_used += sizeof(type); \
} while(0)

static void
reset_message_buffer()
{
    temp_buffer_used = 0;
}

static void
reset_recv_buffer()
{
    recv_buffer_used = 0;
    recv_buffer_size = 0;
}

static bool32
initialize_buffers()
{
    recv_buffer = custom_malloc(MAX_NETWORK_BUFFER_SIZE);
    temp_buffer = custom_malloc(MAX_NETWORK_BUFFER_SIZE);
    
    return (recv_buffer && temp_buffer);
}

static void
terminate_buffers()
{
    custom_free(recv_buffer);
    custom_free(temp_buffer);
}

int32
StartServer()
{
    // this client is hosting the media file
    for(int i = 0; i < ArrayCount(message_ports) && server==NULL; i++)
    {
        IPaddress ip;
        SDLNet_ResolveHost(&ip, NULL, message_ports[i]);
        server = SDLNet_TCP_Open(&ip);
    }
    
    if(server == NULL)
        RETURN(SOCKET_CREATION_FAIL);
    
    socket_set = SDLNet_AllocSocketSet(1);
    is_host = 1;
    
    if(initialize_buffers())
        RETURN(SUCCESS);
    else
        RETURN(NO_MEMORY);
}

int32
StartClient()
{
    socket_set = SDLNet_AllocSocketSet(1);
    is_client = 1;
    
    if(initialize_buffers())
        RETURN(SUCCESS);
    else
        RETURN(NO_MEMORY);
}

int32
CloseServer()
{
    if(server)
    {
        SDLNet_TCP_Close(server);
    }
    if(partner)
    {
        SDLNet_TCP_Close(partner);
    }
    if(socket_set)
    {
        SDLNet_FreeSocketSet(socket_set);
    }
    
    terminate_buffers();
    is_host = 0;
    
    RETURN(SUCCESS);
}

int32
CloseClient()
{
    if(partner)
    {
        SDLNet_TCP_Close(partner);
    }
    if(socket_set)
    {
        SDLNet_FreeSocketSet(socket_set);
    }
    
    terminate_buffers();
    is_client = 0;
    
    RETURN(SUCCESS);
}

int32
AcceptConnection()
{
    if((partner  = SDLNet_TCP_Accept(server)))
    {
        SDLNet_TCP_AddSocket(socket_set, partner);
        RETURN(CONNECTED);
    }
    
    RETURN(SUCCESS);
}

int32
ConnectToIP(const char *ip)
{
    for(int i = 0; i < ArrayCount(message_ports) && partner==NULL; i++)
    {
        IPaddress ip_address;
        SDLNet_ResolveHost(&ip_address, ip, message_ports[i]);
        
        partner = SDLNet_TCP_Open(&ip_address);
    }
    
    if(partner == NULL)
        RETURN(CONNECTION_FAILED);
    
    SDLNet_TCP_AddSocket(socket_set, partner);
    SendInitRequestMessage();
    
    RETURN(SUCCESS);
}

static int32
QueueControlMessage(void *buffer, int32 size)
{
    memcpy(temp_buffer+temp_buffer_used, buffer, size);
    temp_buffer_used += size;
    
    RETURN(SUCCESS);
}

int32
SendControlMessages()
{
    if(temp_buffer_used == 0)
        RETURN(SUCCESS);
    
    void *buffer = custom_malloc(MAX_NETWORK_BUFFER_SIZE);
    //int8 buffer[MAX_NETWORK_BUFFER_SIZE];
    
    int32 temp;
    SDLNet_Write32(temp_buffer_used, &temp);
    
    SDLNet_TCP_Send(partner, &temp, sizeof(temp));
    
    for(int i = 0; i < temp_buffer_used; i+=4)
    {
        int temp = *((int32 *)(temp_buffer+i));
        SDLNet_Write32(temp, buffer+i);
    }
    
    if(SDLNet_TCP_Send(partner, buffer, temp_buffer_used) <= 0)
    {
        custom_free(buffer);
        RETURN(DISCONNECTED);
    }
    /*
    dbg_print("SendControlMessages():\n"
              "\tSize: %d\n"
              "\tConverted: %d\n"
              "\tType: %d\n"
              "\tConverted: %d\n",
              temp_buffer_used,
              temp,
              *((int32 *)temp_buffer),
              *((int32 *)buffer));
    */
    
    custom_free(buffer);
    reset_message_buffer();
    
    RETURN(SUCCESS);
}

/**
Message must be freed after using
*/
net_message *
GetNextMessage()
{
    if(recv_buffer_size == 0)
        return NULL;
    
    net_message *msg = (net_message *)(recv_buffer + recv_buffer_used);
    net_message *new_msg = NULL;
    
    switch(msg->type)
    {
        case MESSAGE_INIT:
        {
            copy_msg(struct _init_msg, new_msg, msg);
            print_init_msg((struct _init_msg *)msg, 1);
        } break;
        case MESSAGE_REQUEST_INIT:
        {
            copy_msg(struct _request_init_msg, new_msg, msg);
            print_request_init_msg((struct _request_init_msg *)msg, 1);
        } break;
        case MESSAGE_REQUEST_PORT:
        {
            copy_msg(struct _request_port_msg, new_msg, msg);
            
        } break;
        case MESSAGE_INFO:
        {
            copy_msg(struct _request_info_msg, new_msg, msg);
            
        } break;
        case MESSAGE_PAUSE:
        {
            copy_msg(struct _pause_msg, new_msg, msg);
            
        } break;
        case MESSAGE_PLAY:
        {
            copy_msg(struct _play_msg, new_msg, msg);
            
        } break;
        case MESSAGE_SEEK:
        {
            copy_msg(struct _seek_msg, new_msg, msg);
            
        } break;
        case MESSAGE_DISCONNECT:
        {
            copy_msg(struct _disconnect_msg, new_msg, msg);
            
        } break;
    }
    
    if(recv_buffer_used >= recv_buffer_size)
        reset_recv_buffer();
    
    return new_msg;
}

int32
ReceiveControlMessages()
{
    int ret = SDLNet_CheckSockets(socket_set, 0);
    if(ret > 0)
    {
        if(SDLNet_SocketReady(partner))
        {
            void *buffer = custom_malloc(MAX_NETWORK_BUFFER_SIZE);
            //int8 buffer[MAX_NETWORK_BUFFER_SIZE];
            
            int32 temp;
            int32 ret = SDLNet_TCP_Recv(partner, &temp, sizeof(temp));
            if(ret <= 0)
                RETURN(DISCONNECTED);
            
            recv_buffer_size = SDLNet_Read32(&temp);
            recv_buffer_used = 0;
            
            SDLNet_TCP_Recv(partner, buffer, recv_buffer_size);
            
            for(int i = 0; i < recv_buffer_size; i+=4)
            {
                int temp = SDLNet_Read32(buffer+i);
                *((int32 *)(recv_buffer+i)) = temp;
            }
        }
    }
    else if(ret < 0)
    {
        // TODO(Val): This is an error and need to handle it
        dbg_error("ReceiveControlMesssages(): SDLNet_CheckSockets() error.\n");
        RETURN(UNKNOWN_ERROR);
    }
    
    RETURN(SUCCESS);
}

void
GetPartnerIP(char **buffer)
{
    IPaddress *temp_ip = SDLNet_TCP_GetPeerAddress(partner);
    
    IPv4 ip;
    ip.ip = SDLNet_Read32(&temp_ip->host);
}

int32
SendInitMessage(real64 start_timestamp,
                real64 file_duration,
                int32 audio_port,
                int32 video_port,
                int32 subtitle_port,
                int32 flags)
{
    Qmsg_init(MESSAGE_INIT, struct _init_msg, msg);
    
    msg.flags = flags;
    msg.start_time = start_timestamp;
    msg.file_duration = file_duration;
    msg.audio_port = audio_port;
    msg.video_port = video_port;
    msg.subtitle_port = subtitle_port;
    
    print_init_msg(&msg, 0);
    
    Qmsg_send(msg);
    
}

int32
SendInitRequestMessage()
{
    Qmsg_init(MESSAGE_REQUEST_INIT, struct _request_init_msg, msg);
    
    print_request_init_msg(&msg, 0);
    
    Qmsg_send(msg);
}

int32
SendRequestPortMessage()
{
    Qmsg_init(MESSAGE_REQUEST_PORT, struct _request_port_msg, msg);
    
    Qmsg_send(msg);
}

int32
SendPlayMessage()
{
    Qmsg_init(MESSAGE_PLAY, struct _play_msg, msg);
    
    print_play_msg(&msg, 0);
    
    Qmsg_send(msg);
}

int32
SendPauseMessage()
{
    Qmsg_init(MESSAGE_PAUSE, struct _pause_msg, msg);
    
    print_pause_msg(&msg, 0);
    
    Qmsg_send(msg);
}

int32
SendSeekMessage(real64 timestamp)
{
    Qmsg_init(MESSAGE_SEEK, struct _seek_msg, msg);
    
    msg.time = timestamp;
    
    print_seek_msg(&msg, 0);
    
    Qmsg_send(msg);
}

int32
SendDisconnectMessage()
{
    Qmsg_init(MESSAGE_DISCONNECT, struct _disconnect_msg, msg);
    
    Qmsg_send(msg);
}

int32
CloseConnection()
{
    SDLNet_TCP_Close(partner);
    
    if(is_host)
        SDLNet_TCP_Close(server);
    
    RETURN(SUCCESS);
}