/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef TIME_H
#define TIME_H

#include "defines.h"

internal real64 PlatformGetTime();
internal int32 PlatformSleep(real64);
internal int32 WaitUntil(real64 time, real64 permissible_buffer);

#endif // TIME_H