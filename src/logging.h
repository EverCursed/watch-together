#ifndef LOGGING_H
#define LOGGING_H

#include <signal.h>
#include "defines.h"

typedef enum log_level {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE,
} log_level;

char *log_level_str[] = {
    "  [ FATAL ]",
    "  [ ERROR ]",
    "[ WARNING ]",
    "   [ INFO ]",
    "  [ DEBUG ]",
    "  [ TRACE ]",
};

#define MAX_LINE_SIZE 256

typedef struct log_buffer {
    sig_atomic_t current_buffer;
    sig_atomic_t log_lines;
    log_level    max_level;
    char         buffer[2][1024][MAX_LINE_SIZE];
} log_buffer;


internal void
InitializeLogger(log_level level, const char* file);

internal void
CloseLogger();

internal void
wlog(log_level level, const char* string);

#endif //LOGGING_H
