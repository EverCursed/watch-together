#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG

#include <stdio.h>

#if defined(_WIN32)
#include <Windows.h>
#define CHANGE_COLOR(x) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x)

#define dbg_print(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#define dbg_error(...) \
do { \
    CHANGE_COLOR(FOREGROUND_RED); \
    fprintf(stderr, "%s %d : ", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__); \
    CHANGE_COLOR(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN); \
} while(0)

#define dbg_info(...) \
do { \
    CHANGE_COLOR(FOREGROUND_BLUE | FOREGROUND_GREEN); \
    fprintf(stderr, __VA_ARGS__); \
    CHANGE_COLOR(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN); \
} while(0)

#define dbg_success(...) \
do { \
    CHANGE_COLOR(FOREGROUND_GREEN); \
    fprintf(stderr, __VA_ARGS__); \
    CHANGE_COLOR(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN); \
} while(0)

#ifndef SUPPRESS_WARN
#define dbg_warn(...) \
do { \
    CHANGE_COLOR(FOREGROUND_RED | FOREGROUND_GREEN); \
    fprintf(stderr, __VA_ARGS__); \
    CHANGE_COLOR(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN); \
} while(0)
#else
#define dbg_warn(...) \
do { } while(0)
#endif // SUPPRESS_WARN

#define dbg(x) x

#elif defined(__linux__)

#define dbg_print(...) \
do {\
    fprintf(stderr, __VA_ARGS__);\
} while(0)

#define dbg_error(...)\
do {\
    fprintf(stderr, KRED);\
    fprintf(stderr, "%s %d : ", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, KNRM);\
} while(0)

#define dbg_info(...) \
do {\
    fprintf(stderr, KCYN);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, KNRM);\
} while(0)

#define dbg_success(...) \
do {\
    fprintf(stderr, KGRN);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, KNRM);\
} while(0)

#ifndef SUPPRESS_WARN
#define dbg_warn(...) \
do { \
    fprintf(stderr, KYEL);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, KNRM);\
} while(0)
#else
#define dbg_warn(...) \
do { } while(0)
#endif // SUPPRESS_WARN

#define dbg(x) x
#endif // OS

#else

#define dbg_print(...) do {} while(0)
#define dbg_error(...) do {} while(0)
#define dbg_info(...) do {} while(0)
#define dbg_success(...) do {} while(0)
#define dbg_warn(...) do {} while(0)
#define dbg(x) do {} while(0)

#endif // DEBUG
#endif // DEBUG_H