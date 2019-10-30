#ifndef MEDIA_PROCESSING_H
#define MEDIA_PROCESSING_H

#include "encoding.h"
#include "decoding.h"
#include "file_data.h"

int32 MediaThreadStart(void *);
int32 MediaOpen(open_file_info *, decoder_info *, encoder_info *);
int32 MediaClose(open_file_info *, decoder_info *, encoder_info *);

#endif // MEDIA_PROCESSING_H