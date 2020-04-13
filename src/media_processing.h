/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef MEDIA_PROCESSING_H
#define MEDIA_PROCESSING_H

#include "encoding.h"
#include "decoding.h"
#include "file_data.h"
#include "audio.h"
#include "video.h"

internal i32 MediaThreadStart(void *);
internal i32 MediaOpen(open_file_info *, decoder_info *, encoder_info *, output_audio *, output_video *);
internal i32 MediaClose(open_file_info *, decoder_info *, encoder_info *, output_audio *, output_video *);

#endif // MEDIA_PROCESSING_H