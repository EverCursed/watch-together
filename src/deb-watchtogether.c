#include <gtk/gtk.h>

#include "version.h"


static void
activate (GtkApplication* app,
          gpointer        user_data)
{
    GtkWidget *window;
    
    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), WT_WINDOW_TITLE);
    // TODO(Val): Make this use stored values in .ini
    gtk_window_set_default_size (GTK_WINDOW (window), 1024, 576);
    gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
    GtkApplication *app;
    int status;
    
    // TODO(Val): Figure out how to properly define an id for the app
    app = gtk_application_new ("com.github.EverCursed.watch-together", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    
    return status;
}