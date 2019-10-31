#ifndef TIME_H
#define TIME_H

#include "defines.h"

real64 PlatformGetTime();
int32 PlatformSleep(real64);
int32 WaitUntil(real64 time, real64 permissible_buffer);

#endif // TIME_H