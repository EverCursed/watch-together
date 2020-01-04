/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef WT_PLATFORM
#define WT_PLATFORM

#include <libavutil/frame.h>
#include "SDL_Platform.h"
#include "file_data.h"

typedef struct _platform_data platform_data;
typedef struct _thread_info thread_info;
typedef struct _cond_info cond_info;
typedef struct _platform_mutex platform_mutex;
typedef struct _output_audio output_audio;
typedef struct _output_video output_video;

internal int32
PlatformFlipBuffers();

internal int
PlatformGetInput();

internal void
PlatformPauseAudio(bool32);

internal int32
PlatformUpdateVideoFrame(AVFrame *frame);

internal int32
PlatformRender();


// Threads 

internal thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

internal void
PlatformWaitThread(thread_info, int32 *);

internal cond_info
PlatformCreateConditionVar();

internal int32
PlatformConditionWait(cond_info *);

internal int32
PlatformConditionSignal(cond_info *);

internal bool32
PlatformConditionDestroy(cond_info *);

internal platform_mutex
PlatformCreateMutex();

internal int32
PlatformLockMutex(platform_mutex *);

internal int32
PlatformUnlockMutex(platform_mutex *);

internal void
PlatformDestroyMutex(platform_mutex *);

internal int32
PlatformInitAudio(open_file_info *);

internal void
PlatformCloseAudio();

internal void
PlatformInitVideo(open_file_info *);

internal void
PlatformToggleFullscreen();

internal int32
PlatformResizeClientArea(open_file_info *, int32, int32);

internal int32
PlatformQueueAudio();

internal int64
PlatformGetThreadID();

#endif