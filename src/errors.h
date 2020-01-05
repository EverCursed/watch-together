/*
This file is part of WatchTogether.
Copyright (C) 2019-2020 Valentine Yelizarov
https://github.com/EverCursed

Function debug return macros.
*/

#ifndef ERRORS_H
#define ERRORS_H

#define SUCCESS              ( 0)
#define CONNECTED            ( 1)

#define UNKNOWN_ERROR        (-1)
#define NO_MEMORY            (-2)
#define NOT_ENOUGH_DATA      (-3)
#define UNKNOWN_FORMAT       (-4)
#define WRONG_ARGS           (-5)
#define CONTAINER_FULL       (-6)
#define FILE_EOF             (-7)
#define FILE_NOT_FOUND       (-8)
#define NOT_INITIALIZED      (-9)
#define NOT_YET_IMPLEMENTED  (-10)
#define FAILED_TO_INIT       (-11)
#define CONNECTION_FAILED    (-12)
#define SOCKET_CREATION_FAIL (-13)
#define SENDING_ERROR        (-14)
#define NEED_DATA            (-15)
#define CONTAINER_EMPTY      (-16)
#define DISCONNECTED         (-17)
#define UNKNOWN_STREAM       (-18)
#define UNINITIALIZED        (-19)

#define success(a) (a >= SUCCESS)
#define fail(a) (a < SUCCESS)

#ifdef DEBUG

#include "debug.h"

#define RETURN(a)\
do {\
    if(a < SUCCESS)\
    dbg_error("%s\t%s\n", __func__, #a);\
    if(a > SUCCESS)\
    dbg_success("%s\t%s\n", __func__, #a);\
    \
    return a;\
} while(0)

#else

#define RETURN(a) do { return a; } while(0)

#endif

#endif // ERRORS_H
