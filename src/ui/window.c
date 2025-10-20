#include "ariel.h"

ArielWindow *
ariel_window_new(ArielApp *app)
{
    ArielWindow *window;
    
    window = g_object_new(GTK_TYPE_APPLICATION_WINDOW,
                          "application", app,
                          "title", APP,
                          "default-width", 1200,
                          "default-height", 800,
                          NULL);
                          
    window->app = app;
    ariel_window_setup_ui(window);

    gtk_window_maximize(GTK_WINDOW(window));
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
    // gtk_box_append(GTK_BOX(vbox), window->transport_box);
    
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
    window->active_plugins = ariel_create_active_plugins_view(window);
    gtk_paned_set_start_child(GTK_PANED(right_paned), window->active_plugins);
    gtk_widget_set_name(window->active_plugins, "active-plugins-view");
    
    // Mixer (bottom right)
    window->mixer_box = ariel_create_mixer(window);
    // gtk_paned_set_end_child(GTK_PANED(right_paned), window->mixer_box);
    
    // Set paned positions
    gtk_paned_set_position(GTK_PANED(window->main_paned), 400);
    gtk_paned_set_position(GTK_PANED(right_paned), 400);
    
    // Auto-start the audio engine
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    if (ariel_audio_engine_start(engine)) {
        gtk_button_set_label(GTK_BUTTON(window->audio_toggle), "Audio: ON");
        gtk_widget_add_css_class(GTK_WIDGET(window->audio_toggle), "suggested-action");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window->audio_toggle), TRUE);
        g_print("Audio engine auto-started successfully\n");
    } else {
        g_warning("Failed to auto-start audio engine");
    }
}