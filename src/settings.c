#include "defines.h"
#include "settings.h"
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/*
- settings.ini -
window-width:1024
window-height:576
port:52844
username:user
*/

internal
char* home_path()
{
    return getenv("HOME");
}

wt_settings* init_settings()
{
    FILE *f = fopen("~/" FOLDER_NAME "/settings.ini", "r+");
    wt_settings* settings = malloc(sizeof(wt_settings));
    if(f)
    {
        
    }
    else
    {
        settings->window_width = SETTINGS_DEFAULT_WIDTH;
        settings->window_height = SETTINGS_DEFAULT_HEIGHT;
        settings->port = SETTINGS_DEFAULT_PORT;
        strcpy(settings->username, SETTINGS_DEFAULT_USERNAME);
        
        save_settings(settings);
    }
    
    return settings;
}

int save_settings(wt_settings* settings)
{
    char dat[512];
    struct stat st = {0};
    
    char path_home[256];
    sprintf(path_home, "%s/%s", home_path(), FOLDER_NAME);
    char path_settings[256]; 
    sprintf(path_settings, "%s/%s", path_home, "settings.ini");
    
    // probe save folder, if doesn't exist, create it.
    int ret;
    if (stat("~/" FOLDER_NAME, &st) == -1) {
        ret = mkdir(path_home, 0700);
    }
    
    if(ret == 0)
    {
        FILE* file = fopen(path_settings, "w");
        if(file)
        {
            sprintf(dat, "%s:%d\n%s:%d\n%s:%d\n%s:%s\n", 
                    SETTINGS_WIDTH_TITLE, settings->window_width,
                    SETTINGS_HEIGHT_TITLE, settings->window_height,
                    SETTINGS_PORT_TITLE, settings->port,
                    SETTINGS_USERNAME_TITLE, settings->username
                    );
            fwrite(dat, sizeof(char), strlen(dat), file);
            fclose(file);
            return 1;
        }
        else
        {
            printf("Failed creating save file.");
            return 0;
        }
    }
    else 
        return 0;
}