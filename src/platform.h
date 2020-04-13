/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef WT_PLATFORM
#define WT_PLATFORM

#include <libavutil/frame.h>
#include <SDL2/SDL.h>
#include "SDL_Platform.h"
#include "file_data.h"

typedef struct _platform_data platform_data;
typedef struct _thread_info thread_info;
typedef struct _cond_info cond_info;
typedef struct _platform_mutex platform_mutex;
typedef struct _output_audio output_audio;
typedef struct _output_video output_video;

internal i32
PlatformFlipBuffers();

internal int
PlatformGetInput();

internal void
PlatformPauseAudio(b32);

internal i32
PlatformUpdateVideoFrame(AVFrame *frame);

internal i32
PlatformRender();


// Threads 

internal thread_info
PlatformCreateThread(i32 (*f)(void *), void *, char *);

internal void
PlatformWaitThread(thread_info, i32 *);

internal cond_info
PlatformCreateConditionVar();

internal i32
PlatformConditionWait(cond_info *);

internal i32
PlatformConditionSignal(cond_info *);

internal b32
PlatformConditionDestroy(cond_info *);

internal platform_mutex
PlatformCreateMutex();

internal i32
PlatformLockMutex(platform_mutex *);

internal i32
PlatformUnlockMutex(platform_mutex *);

internal void
PlatformDestroyMutex(platform_mutex *);

internal i32
PlatformInitAudio(open_file_info *);

internal void
PlatformCloseAudio();

internal void
PlatformInitVideo(open_file_info *);

internal void
PlatformChangeFullscreenState(b32 on);

//internal i32
//PlatformResizeClientArea(open_file_info *, i32, i32);

internal i32
PlatformQueueAudio();

internal i64
PlatformGetThreadID();

internal SDL_Texture *
PlatformConvertSurfaceToTexture(SDL_Surface *surface);
#endif