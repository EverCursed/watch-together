#ifndef WT_SETTINGS_H

#define SETTINGS_PORT_TITLE "port"
#define SETTINGS_WIDTH_TITLE "window-width"
#define SETTINGS_HEIGHT_TITLE "window-height"
#define SETTINGS_USERNAME_TITLE "username"

#define WT_SETTINGS_H
#define SETTINGS_DEFAULT_PORT 52844
#define SETTINGS_DEFAULT_WIDTH 1024
#define SETTINGS_DEFAULT_HEIGHT 576
#define SETTINGS_DEFAULT_USERNAME "user"

typedef struct wt_settings {
    int window_width;
    int window_height;
    int port;
    
    // TODO: Make this more flexible
    char username[64];
} wt_settings;

#endif