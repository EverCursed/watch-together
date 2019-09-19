#ifndef ERRORS_H
#define ERRORS_H

typedef int err_code;

#define SUCCESS              ( 0)

#define UNKNOWN_ERROR        (-1)
#define NO_MEMORY            (-2)
#define NOT_ENOUGH_DATA      (-3)
#define UNKNOWN_FORMAT       (-4)
#define WRONG_ARGS           (-5)
#define CONTAINER_FULL       (-6)
#define FILE_EOF             (-7)
#define FILE_NOT_FOUND       (-8)
#define NOT_INITIALIZED      (-9)

#define s(a) (a >= SUCCESS)
#define f(a) (a < SUCCESS)

#ifdef DEBUG

#include "debug.h"

#define RETURN(a)\
do {\
    if(a == SUCCESS)\
    return a;\
    if(a < SUCCESS)\
    dbg_error("%s\t%s\n", __func__, #a);\
    return a;\
} while(0)

#else

#define RETURN(a) do { return a } while(0)

#endif

#endif
