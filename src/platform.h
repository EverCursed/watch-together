#ifndef WT_PLATFORM
#define WT_PLATFORM

static uint32
PlatformGetTime();

static int
PlatformGetInput(program_data *);

static void
PlatformPauseAudio(bool32);

static int32
PlatformUpdateFrame(program_data *);

static void
PlatformSleep(int32);

static thread_info
PlatformCreateThread(int32 (*f)(void *), void *, char *);

static int32
PlatformWaitThread(thread_info, int32 *);

static void
PlatformInitAudio(program_data *);

static void
PlatformCloseAudio(program_data *);

static void
PlatformInitVideo(program_data *);

static void
PlatformToggleFullscreen(program_data *);
#endif