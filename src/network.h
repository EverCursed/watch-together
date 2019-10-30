#ifndef NETWORK_H
#define NETWORK_H

#include "defines.h"

typedef union _IPv4{
    struct{
        uint8 a;
        uint8 b;
        uint8 c;
        uint8 d;
    };
    struct {
        uint8 octet[4];
    };
    uint32 ip;
} IPv4;

typedef union _IPv6{
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
        };
        struct {
            IPv6 v6;
        };
    };
    bool32 is_ipv6;
} destination_IP;

int32 StartServer();
int32 AcceptConnection();
int32 ConnectToIP(destination_IP ip);
int32 SendControlMessage(int32, int32, ...);
int32 CloseConnection();
int32 CloseServer();

#endif 