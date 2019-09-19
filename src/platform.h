#ifndef WT_PLATFORM
#define WT_PLATFORM

#include "watchtogether.h"

real64
PlatformGetTime();

int32
PlatformFlipBuffers(program_data *);

int
PlatformGetInput(program_data *);

void
PlatformPauseAudio(bool32);

int32
PlatformUpdateVideoFrame(program_data *);

int32
PlatformUpdateFrame(program_data *);

void
PlatformSleep(real64);


// Threads 

thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

void
PlatformWaitThread(thread_info, int32 *);

cond_info
PlatformCreateConditionVar();

int32
PlatformConditionWait(cond_info *);

int32
PlatformConditionSignal(cond_info *);

bool32
PlatformConditionDestroy(cond_info *);

platform_mutex
PlatformCreateMutex();

int32
PlatformLockMutex(platform_mutex *);

int32
PlatformUnlockMutex(platform_mutex *);

int32
PlatformInitAudio(program_data *);

void
PlatformCloseAudio(program_data *);

void
PlatformInitVideo(program_data *);

void
PlatformToggleFullscreen(program_data *);

int32
PlatformQueueAudio(output_audio *);

int64
PlatformGetThreadID();

#endif