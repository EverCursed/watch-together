#include <sys/stat.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "deb-defines.h"
#include "defines.h"
#include "version.h"
#include "settings.h"
#include "errno.h"

/*
List of things the platform code must provide:

Menu:
 X File
  X | Open
  X | Quit
 - Connection
 - | Host
 - | Connect to Partner
 - | Statistics
 - Settings
 - | Preferences
 - Help
 - | Check updates
   X | About
   
  Following services the platform code must provide: 
  
  - Sending/Receiving UDP packets
  - Opening file
  ~ Displaying video
  - Interpreting controls
  
*/

// returns home path
internal
char* home_path() { return getenv("HOME"); }

/************************************************

Settings Saving/Reading

************************************************/

int save_file_raw(char* filename, char* raw_text)
{
    FILE* f = fopen(filename, "w");
    debug();
    if(f)
    {
        fwrite(raw_text, sizeof(char), strlen(raw_text), f);
        fclose(f);
        debug();
        return 1;
    }
    else
    {
        debug();
        return 0;
    }
}

int save_settings_raw(char* raw_text)
{
    char path_settings[256]; 
    sprintf(path_settings, "%s/%s/%s", home_path(), FOLDER_NAME, "settings.ini");
    debug();
    return save_file_raw(path_settings, raw_text);
}

// returns the loaded file to the caller
// caller is responsible for freeing later
char* load_settings_raw()
{
    char settings_file[256];
    sprintf(settings_file, "%s/%s/%s", home_path(), FOLDER_NAME, "settings.ini");
    
    FILE *f = fopen(settings_file, "r");
    debug();
    if(!f)
        return NULL;
    
    char* settings_raw = malloc(sizeof(char) * SETTINGS_MAX_LENGTH);
    
    debug();
    
    int n = fread(settings_raw, sizeof(char), SETTINGS_MAX_LENGTH-1, f);
    *(settings_raw + n)= '\0';
    
    fclose(f);
    debug();
    return settings_raw;
}

int check_folder_exists()
{
    struct stat st = {0};
    
    char save_folder[256];
    sprintf(save_folder, "%s/%s", home_path(), FOLDER_NAME);
    if (stat(save_folder, &st) == -1) {
        int ret;
        ret = mkdir(save_folder, 0700);
        printf("mkdir() returned %d, errno: %d\n", ret, errno);
        debug();
        return ret + 1;
    }
    printf("stat() returned 0\n");
    debug();
    return 1;
}

/************************************************

GTK Window Set Up

************************************************/

void menuitem_quit(GtkMenuItem *menuitem, gpointer data)
{
    // TODO(Val): Close any open files
    
    g_application_quit(G_APPLICATION(data));
}

void menuitem_display_about(GtkMenuItem *menuitem, gpointer data)
{
    GtkWindow *window = data;
    gtk_show_about_dialog(window,
                          "program-name", "WatchTogether",
                          "version", WT_FULL_VERSION,
                          NULL);
}

void menuitem_open_file(GtkMenuItem *menuitem, gpointer data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open File",
                                                     GTK_WINDOW(data),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "Cancel",
                                                     GTK_RESPONSE_CANCEL,
                                                     "Open",
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename (chooser);
        // TODO(Val): Open file
    }
    
    gtk_widget_destroy (dialog);
}

// initialize the menu bar
static GtkWidget* init_menubar(GtkApplication *app, GtkWindow *window)
{
    GtkWidget *menubar;
    GtkWidget *sep;
    
    GtkWidget *menu_file;
    GtkWidget *menu_file_menu;
    GtkWidget *menu_file_open;
    GtkWidget *menu_file_quit;
    
    GtkWidget *menu_connection;
    GtkWidget *menu_connection_menu;
    GtkWidget *menu_connection_host;
    GtkWidget *menu_connection_connect;
    GtkWidget *menu_connection_stats;
    
    GtkWidget *menu_settings;
    GtkWidget *menu_settings_menu;
    GtkWidget *menu_settings_prefs;
    
    GtkWidget *menu_help;
    GtkWidget *menu_help_menu;
    GtkWidget *menu_help_update;
    GtkWidget *menu_help_about;
    
    
    menubar = gtk_menu_bar_new();
    sep = gtk_separator_menu_item_new();
    
    // initialize menubar items
    menu_file = gtk_menu_item_new_with_label("File");
    menu_connection = gtk_menu_item_new_with_label("Connection");
    menu_settings = gtk_menu_item_new_with_label("Settings");
    menu_help = gtk_menu_item_new_with_label("Help");
    
    // attach items to menubar
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_file);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_connection);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_settings);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_help);
    
    // File dropdown menu
    menu_file_menu = gtk_menu_new();
    menu_file_open = gtk_menu_item_new_with_label("Open");
    menu_file_quit = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file_menu),
                          menu_file_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file_menu),
                          sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_file_menu),
                          menu_file_quit);
    
    // Connection dropdown menu
    menu_connection_menu = gtk_menu_new();
    menu_connection_host = gtk_menu_item_new_with_label("Host");
    menu_connection_connect = gtk_menu_item_new_with_label("Connect to Partner");
    menu_connection_stats = gtk_menu_item_new_with_label("Statistics");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_connection_menu),
                          menu_connection_host);gtk_menu_shell_append(GTK_MENU_SHELL(menu_connection_menu),
                                                                      menu_connection_connect);gtk_menu_shell_append(GTK_MENU_SHELL(menu_connection_menu),
                                                                                                                     menu_connection_stats);
    
    // Settings dropdown menu
    menu_settings_menu = gtk_menu_new();
    menu_settings_prefs = gtk_menu_item_new_with_label("Preferences");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_settings_menu), menu_settings_prefs);
    
    // Help drowdown menu
    menu_help_menu = gtk_menu_new();
    menu_help_update = gtk_menu_item_new_with_label("Check for Updates");
    menu_help_about = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_help_menu),
                          menu_help_update);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_help_menu),
                          menu_help_about);
    
    
    // attach menus to the menubar items
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_file), menu_file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_connection), menu_connection_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_settings), menu_settings_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_help), menu_help_menu);
    
    g_signal_connect(G_OBJECT(menu_file_quit),
                     "activate",
                     G_CALLBACK(menuitem_quit),
                     app);
    g_signal_connect(G_OBJECT(menu_help_about),
                     "activate",
                     G_CALLBACK(menuitem_display_about),
                     window);
    g_signal_connect(G_OBJECT(menu_file_open),
                     "activate",
                     G_CALLBACK(menuitem_open_file),
                     window);
    
    return menubar;
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
    
    wt_settings* settings = init_settings();
    
    GtkWidget *window;
    GtkWidget *vbox;
    
    window = gtk_application_window_new (app);
    gtk_window_set_title(GTK_WINDOW (window), WT_WINDOW_TITLE);
    
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    GtkWidget *menubar = 
        init_menubar(app, GTK_WINDOW(window));
    
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    
    g_assert(GTK_IS_WINDOW(window));
    
    gtk_window_set_default_size(GTK_WINDOW (window), 
                                atoi(settings_get(settings, SETTINGS_WIDTH)),
                                atoi(settings_get(settings, SETTINGS_HEIGHT))
                                );
    gtk_widget_show_all(window);
}

int main (int argc, char **argv)
{
    GtkApplication *app;
    int status;
    
    app = gtk_application_new ("com.github.EverCursed.watchtogether", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    
    
    g_object_unref (app);
    
    // TODO(Val): Close open file.
    
    return status;
}