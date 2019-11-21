#ifndef STREAMING_H
#define STREAMING_H

#include <libavformat/avformat.h>
#include "defines.h"
#include "decoding.h"

int32
Streaming_Host_SendPacket(decoder_info *, AVPacket *);

int32
Streaming_Host_Initialize(decoder_info *);

int32
Streaming_Host_Close();

int32
StreamPackets();

#endif // STREAMING_H