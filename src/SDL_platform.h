/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed


*/
 
#ifndef DEB_WT
#define DEB_WT

#include <SDL2/SDL.h>
#include "defines.h"

typedef struct _platform_data {
    
} platform_data;

typedef struct _thread_info {
    SDL_Thread *thread;
} thread_info;

typedef struct _cond_info {
    SDL_cond *cond;
    SDL_mutex *mutex;
    bool32 test;
} cond_info;

typedef struct _platform_mutex {
    SDL_mutex *mutex;
} platform_mutex;

#endif