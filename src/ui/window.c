#include "ariel.h"

ArielWindow *
ariel_window_new(ArielApp *app)
{
    ArielWindow *window;
    
    window = g_object_new(GTK_TYPE_APPLICATION_WINDOW,
                          "application", app,
                          "title", "Ariel LV2 Host",
                          "default-width", 1200,
                          "default-height", 800,
                          NULL);
                          
    window->app = app;
    ariel_window_setup_ui(window);
    
    return window;
}

void
ariel_window_setup_ui(ArielWindow *window)
{
    GtkWidget *vbox;
    
    // Create main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    
    // Create header bar
    window->header_bar = ariel_create_header_bar(window);
    gtk_window_set_titlebar(GTK_WINDOW(window), window->header_bar);
    
    // Create transport controls
    window->transport_box = ariel_create_transport(window);
    gtk_box_append(GTK_BOX(vbox), window->transport_box);
    
    // Create main paned view
    window->main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(window->main_paned, TRUE);
    gtk_box_append(GTK_BOX(vbox), window->main_paned);
    
    // Create plugin list (left pane)
    window->plugin_list = ariel_create_plugin_list(window);
    gtk_paned_set_start_child(GTK_PANED(window->main_paned), window->plugin_list);
    
    // Create right pane with active plugins and mixer
    GtkWidget *right_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_end_child(GTK_PANED(window->main_paned), right_paned);
    
    // Active plugins list (top right)
    window->active_plugins = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->active_plugins),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_set_start_child(GTK_PANED(right_paned), window->active_plugins);
    
    // Mixer (bottom right)
    window->mixer_box = ariel_create_mixer(window);
    gtk_paned_set_end_child(GTK_PANED(right_paned), window->mixer_box);
    
    // Set paned positions
    gtk_paned_set_position(GTK_PANED(window->main_paned), 300);
    gtk_paned_set_position(GTK_PANED(right_paned), 400);
}