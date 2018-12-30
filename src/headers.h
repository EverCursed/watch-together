#ifndef WT_HEADERS_H
#define WT_HEADERS_H

#if defined(_WIN32) || defined(_WIN64)
#include "win32-watchtogether.c"
#elif defined(unix) || defined(__unix) || defined(__unix__)
#include "deb-watchtogether.c"
#elif defined(__APPLE__) || defined(__MACH__)
#include "os-watchtogether.c"
#endif

#endif 