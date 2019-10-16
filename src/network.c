#include "network.h"

#include "utils.h"

#include <stdarg.h>

#include "SDL2/SDL_net.h"

static uint16 message_ports[4] = {8212, 8418, 23458, 28002};
static uint16 rtp_ports[4] = {9165, 9347, 18669, 21222};

/*

TCPsocket SDLNet_TCP_Open(IPaddress *ip)
void SDLNet_TCP_Close(TCPsocket sock)
TCPsocket SDLNet_TCP_Accept(TCPsocket server)
IPaddress *SDLNet_TCP_GetPeerAddress(TCPsocket sock)
int SDLNet_TCP_Send(TCPsocket sock, const void *data, int len)
int SDLNet_TCP_Recv(TCPsocket sock, void *data, int maxlen)

*/

static TCPsocket here = NULL;
static TCPsocket there = NULL;

int32
StartServer()
{
    // this client is hosting the media file
    for(int i = 0; i < ArrayCount(message_ports) && here==NULL; i++)
    {
        IPaddress ip = {INADDR_ANY, message_ports[i]};
        here = SDLNet_TCP_Open(&ip);
    }
    
    if(here == NULL)
        RETURN(SOCKET_CREATION_FAIL);
    
    RETURN(SUCCESS);
}

int32
AcceptConnection()
{
    if((there = SDLNet_TCP_Accept(here)))
        RETURN(CONNECTED);
    
    RETURN(SUCCESS);
}

int32
ConnectToIP(destination_IP ip)
{
    for(int i = 0; i < ArrayCount(message_ports) && here==NULL; i++)
    {
        IPaddress ip_address = {ip.v4.ip, message_ports[i]};
        there = SDLNet_TCP_Open(&ip_address);
    }
    
    if(there == NULL)
        RETURN(CONNECTION_FAILED);
    
    RETURN(SUCCESS);
}

int32
SendControlMessage(int32 msg, int n, ...)
{
    char buff[256];
    int32 size = 0;
    
    SDLNet_Write32(msg, buff+size);
    size += 4;
    
    //int32 n;
    va_list args;
    va_start(args, n);
    
    SDLNet_Write32(n, buff+size);
    size += 4;
    
    for(int i = 0; i < n; i++ )
    {
        SDLNet_Write32(va_arg(args, int32), buff+size);
        size += 4;
    }
    
    va_end(args);
    
    int ret = SDLNet_TCP_Send(there, buff, size);
    if(ret != size)
        RETURN(SENDING_ERROR);
    
    RETURN(SUCCESS);
}

int32
CloseConnection()
{
    if(there)
        SDLNet_TCP_Close(there);
    
    RETURN(SUCCESS);
}

int32
CloseServer()
{
    if(here)
    {
        SDLNet_TCP_Close(here);
    }
    
    RETURN(SUCCESS);
}

