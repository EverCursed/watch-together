/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/

#ifndef TIME_H
#define TIME_H

#include "defines.h"

internal f64 PlatformGetTime();
internal int32 PlatformSleep(f64);
internal int32 WaitUntil(f64 time, f64 permissible_buffer);

#endif // TIME_H