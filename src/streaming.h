#ifndef STREAMING_H
#define STREAMING_H

#include <libavformat/avformat.h>
#include "defines.h"
#include "decoding.h"
#include "network.h"
#include "file_data.h"

int32
Streaming_Host_SendPacket(decoder_info *, AVPacket *);

int32
Streaming_Host_Initialize(decoder_info *, open_file_info *, char *);

int32
Streaming_Client_Initialize(decoder_info *);

int32
Streaming_Host_Close(decoder_info *);

int32
StreamPackets();

int32
Streaming_Get_Port();



//void Streaming_Send_Header();

int32
Streaming_GetFileName(char *buffer, char *address, int32 video_port, char *parameters);


#endif // STREAMING_H