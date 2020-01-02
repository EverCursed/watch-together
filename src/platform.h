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

static int32
PlatformFlipBuffers();

static int
PlatformGetInput();

static void
PlatformPauseAudio(bool32);

static int32
PlatformUpdateVideoFrame(AVFrame *frame);

static int32
PlatformRender();


// Threads 

static thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

static void
PlatformWaitThread(thread_info, int32 *);

static cond_info
PlatformCreateConditionVar();

static int32
PlatformConditionWait(cond_info *);

static int32
PlatformConditionSignal(cond_info *);

static bool32
PlatformConditionDestroy(cond_info *);

static platform_mutex
PlatformCreateMutex();

static int32
PlatformLockMutex(platform_mutex *);

static int32
PlatformUnlockMutex(platform_mutex *);

static void
PlatformDestroyMutex(platform_mutex *);

static int32
PlatformInitAudio(open_file_info *);

static void
PlatformCloseAudio();

static void
PlatformInitVideo(open_file_info *);

static void
PlatformToggleFullscreen();

static int32
PlatformResizeClientArea(open_file_info *, int32, int32);

static int32
PlatformQueueAudio();

static int64
PlatformGetThreadID();

#endif