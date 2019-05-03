#ifndef WT_PLATFORM
#define WT_PLATFORM

static int32
PlatformFrameUpdater(void *data);

static void
PlatformSleep(uint32);

static thread_info
PlatformCreateThread(int32 (*f)(void *), void*, char*);

static int32
PlatformWaitThread(thread_info, int32 *);

static void
PlatformInitAudio(program_data *);

static void
PlatformInitVideo(program_data *);

#endif