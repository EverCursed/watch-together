#include "defines.h"
/*

List of things the platform code must provide:

Menu:
- File
 - | Open
 - | Quit
 - Connection
 - | Host
 - | Connect to Partner
 - | Statistics
 - Settings
 - | Preferences
 - Help
 - | Check updates
  - | About
  
  Following services the platform code must provide: 
  
  - Sending/Receiving UDP packets
  - Opening file
  ~ Displaying video
  - Interpreting controls
  
*/

#include <gtk/gtk.h>

#include "version.h"

void menuitem_quit(GtkMenuItem *menuitem, gpointer data)
{
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
    
    return menubar;
}


static void
activate (GtkApplication* app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *vbox;
    
    window = gtk_application_window_new (app);
    gtk_window_set_title(GTK_WINDOW (window), WT_WINDOW_TITLE);
    
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    GtkWidget *menubar = init_menubar(app, GTK_WINDOW(window));
    
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    
    g_assert(GTK_IS_WINDOW(window));
    
    // TODO(Val): Make this use stored values in .ini
    gtk_window_set_default_size(GTK_WINDOW (window), 1024, 576);
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
    
    return status;
}