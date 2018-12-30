#ifndef WT_SETTINGS_H

#define SETTINGS_MAX_LENGTH 1024

#define SETTINGS_PORT "port"
#define SETTINGS_WIDTH "win-width"
#define SETTINGS_HEIGHT "win-height"
#define SETTINGS_USERNAME "username"

#define WT_SETTINGS_H
#define SETTINGS_DEFAULT_PORT 52844
#define SETTINGS_DEFAULT_WIDTH 1024
#define SETTINGS_DEFAULT_HEIGHT 576
#define SETTINGS_DEFAULT_USERNAME "user"

typedef struct wt_settings {
    char key[64][64];
    char value[64][64];
    const int size;
    int current_index;
} wt_settings;

wt_settings* init_settings();
char* settings_get(wt_settings*, const char*);
int save_settings(wt_settings*);


#endif