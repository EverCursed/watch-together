#include <stdio.h>
#include <SDL2/SDL.h>
#include "defines.h"
#include "logging.h"

static log_buffer logging_buffer;
static const char* logging_output_file;

struct log_output_data {
    i32 buffer;
    i32 lines;
};

internal int
WriteToFileThread(void *ptr)
{
    struct log_output_data *dat = ptr;
    int b = dat->buffer;
    int l = dat->lines;
    free(ptr);
    
    FILE *log_output_file = fopen(logging_output_file, "a");
    
    for(int i = 0; i < l; i++)
    {
        fputs(logging_buffer.buffer[b][i], log_output_file);
    }
    
    fclose(log_output_file);
    return 0;
}

internal void
OutputLogs()
{
    int b = logging_buffer.current_buffer;
    int l = logging_buffer.log_lines;
    
    logging_buffer.current_buffer = !logging_buffer.current_buffer;
    logging_buffer.log_lines = 0;
    
    struct log_output_data *dat = malloc(sizeof(struct log_output_data));
    dat->buffer = b;
    dat->lines = l;
    
    SDL_CreateThread(WriteToFileThread,
                     "AsyncFileWrite",
                     dat);
}

internal void
InitializeLogger(log_level level, const char* file)
{
    // TODO(Val): add log level filtering
    
    logging_output_file = file;
    
    FILE *log_output_file = fopen(logging_output_file, "w");
    fclose(log_output_file);
    
    logging_buffer.current_buffer = 0;
    logging_buffer.log_lines = 0;
    logging_buffer.max_level = level;
}

internal void
CloseLogger()
{
    OutputLogs();
}

// log format:
//     [ TYPE ] mm/dd/yyyy hh:mm:ss - Thread: <ID> - <string here>
//     [ ERROR ] 02/19/2020 14:11:36 - Thread: 5512 - Queue was not allocated before accessing.
//
//
//
//
internal void
wlog(log_level level, const char* string)
{
    // TODO(Val): add log level filtering
    if(level > logging_buffer.max_level) return;
    
    int old_lines = logging_buffer.log_lines++;
    int old_buffer = logging_buffer.current_buffer;
    int str_len = strlen(string);
    
    time_t timestamp = time(NULL);
    struct tm *time = localtime(&timestamp);
    
    snprintf(logging_buffer.buffer[old_buffer][old_lines], MAX_LINE_SIZE,
             "%s %02d/%02d/%4d %02d:%02d:%02d - Thread: %d - %s\n",
             log_level_str[level],
             time->tm_mon,
             time->tm_mday,
             time->tm_year + 1900,
             time->tm_hour,
             time->tm_min,
             time->tm_sec,
             SDL_ThreadID(),
             string);
    
    // TODO(Val): There is a bug when 900 lines are added before the previous write can be completed. 
    if(logging_buffer.log_lines == 900)
    {
        OutputLogs();
    }
}
