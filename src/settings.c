#include <malloc.h>
#include "deb-watchtogether.h"
#include "settings.h"
#include <string.h>
#include "defines.h"

#define cpy_int_to_str(dest, n) sprintf(dest, "%d", n)

void settings_set(wt_settings* settings, const char* key, char* val)
{
    debug();
    int i;
    for(i = 0; i < settings->current_index && !strcmp(key, settings->key[i]); i++);
    strcpy(settings->value[i], val);
}

char* settings_get(wt_settings* settings, const char* key)
{
    debug();
    for(int i = 0; i < 64 && settings->key[i]; i++)
    {
        if(!strcmp(settings->key[i], key))
            return settings->value[i];
    }
    return NULL;
}

void settings_add_int(wt_settings* settings, const char* key, int value)
{
    strcpy(settings->key[settings->current_index], key);
    cpy_int_to_str(settings->value[settings->current_index], value);
    settings->current_index++;
}

void settings_add_str(wt_settings* settings, const char* key, char* value)
{
    strcpy(settings->key[settings->current_index], key);
    strcpy(settings->value[settings->current_index], value);
    settings->current_index++;
}

// save settings to file
int save_settings(wt_settings* settings)
{
    debug();
    char dat[SETTINGS_MAX_LENGTH] = {0};
    
    // probe save folder, if doesn't exist, create it.
    if(check_folder_exists())
    {
        debug();
        char temp[SETTINGS_MAX_LENGTH];
        for(int i = 0; i < settings->current_index; i++)
        {
            debug();
            sprintf(temp, "%s:%s\n", settings->key[i], settings->value[i]);
            strcat(dat, temp);
        }
        
        printf("%s", dat);
        return save_settings_raw(dat);
    }
    else 
        return 0;
}


// settings initialize / create settings file
wt_settings* init_settings()
{
    debug();
    wt_settings* settings = calloc(1, sizeof(wt_settings));
    *((int *)&(settings->size)) = 64;
    char* dat = load_settings_raw();
    debug();
    // if no settings file found
    if(!dat)
    {
        // TODO(Val): Figure out a better way to set the defaults 
        debug();
        
        settings_add_int(settings, SETTINGS_HEIGHT, SETTINGS_DEFAULT_HEIGHT); 
        settings_add_int(settings, SETTINGS_WIDTH, SETTINGS_DEFAULT_WIDTH);
        settings_add_int(settings, SETTINGS_PORT, SETTINGS_DEFAULT_PORT);
        settings_add_str(settings, SETTINGS_USERNAME, SETTINGS_DEFAULT_USERNAME);
        
        debug();
        
        save_settings(settings);
    }
    else
    {
        // TODO(Val): Think about how to include comments into ini files.
        debug();
        char* tmp = dat;
        tmp = strtok(tmp, ":");
        while(tmp != NULL)
        {
            printf("%s\n", tmp);
            strcpy(settings->key[settings->current_index], tmp);
            tmp = strtok(NULL, "\n");
            
            printf("%s\n", tmp);
            strcpy(settings->value[settings->current_index++], tmp);
            tmp = strtok(NULL, ":");
        }
    }
    debug();
    free(dat);
    return settings;
}