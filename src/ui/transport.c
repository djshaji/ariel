#include "ariel.h"

static void
on_settings_clicked(GtkButton *button, ArielWindow *window)
{
    ariel_show_settings_dialog(window);
}

static void
on_audio_toggle_clicked(GtkToggleButton *button, ArielWindow *window)
{
    gboolean active = gtk_toggle_button_get_active(button);
    
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    if (active) {
        // Check if engine is already running
        if (engine->active) {
            g_print("Audio engine is already running\n");
            return;
        }
        
        if (ariel_audio_engine_start(engine)) {
            gtk_button_set_label(GTK_BUTTON(button), "Audio: ON");
            gtk_widget_add_css_class(GTK_WIDGET(button), "suggested-action");
        } else {
            gtk_toggle_button_set_active(button, FALSE);
            // Show error dialog using modern GtkAlertDialog (GTK 4.10+)
            g_warning("Failed to start audio engine");
        }
    } else {
        ariel_audio_engine_stop(engine);
        gtk_button_set_label(GTK_BUTTON(button), "Audio: OFF");
        gtk_widget_remove_css_class(GTK_WIDGET(button), "suggested-action");
    }
}

GtkWidget *
ariel_create_header_bar(ArielWindow *window)
{
    GtkWidget *header_bar;
    GtkWidget *menu_button;
    
    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header_bar),
                                    gtk_label_new(APP));
    
    // Audio engine toggle button
    window->audio_toggle = gtk_toggle_button_new_with_label("Audio: OFF");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), window->audio_toggle);
    g_signal_connect(window->audio_toggle, "clicked",
                     G_CALLBACK(on_audio_toggle_clicked), window);
    
    // Settings button
    GtkWidget *settings_button = gtk_button_new_from_icon_name("preferences-system-symbolic");
    gtk_widget_set_tooltip_text(settings_button, "Settings");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), settings_button);
    g_signal_connect(settings_button, "clicked",
                     G_CALLBACK(on_settings_clicked), window);
    
    // Menu button
    menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), menu_button);
    
    return header_bar;
}

// Transport control callbacks
static void
on_play_clicked(GtkButton *button, ArielWindow *window)
{
    if (window->is_playing) {
        ariel_transport_stop(window);
    } else {
        ariel_transport_play(window);
    }
}

static void
on_stop_clicked(GtkButton *button, ArielWindow *window)
{
    ariel_transport_stop(window);
}

static void
on_record_clicked(GtkButton *button, ArielWindow *window)
{
    if (window->is_recording) {
        ariel_transport_stop(window);
    } else {
        ariel_transport_record(window);
    }
}

// Transport control functions
void
ariel_transport_play(ArielWindow *window)
{
    if (!window) return;
    
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    if (!engine || !engine->active) {
        g_warning("Cannot start playback - audio engine not running");
        return;
    }
    
    g_print("Transport: Starting playback\n");
    
    window->is_playing = TRUE;
    window->is_recording = FALSE;
    
    // TODO: Start JACK transport if available
    // jack_transport_start(engine->client);
    
    ariel_transport_update_ui(window);
}

void
ariel_transport_stop(ArielWindow *window)
{
    if (!window) return;
    
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    if (!engine || !engine->active) {
        return;
    }
    
    g_print("Transport: Stopping playback/recording\n");
    
    window->is_playing = FALSE;
    window->is_recording = FALSE;
    
    // TODO: Stop JACK transport if available
    // jack_transport_stop(engine->client);
    
    ariel_transport_update_ui(window);
}

void
ariel_transport_record(ArielWindow *window)
{
    if (!window) return;
    
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    if (!engine || !engine->active) {
        g_warning("Cannot start recording - audio engine not running");
        return;
    }
    
    g_print("Transport: Starting recording\n");
    
    window->is_playing = TRUE;
    window->is_recording = TRUE;
    
    // TODO: Start recording to file
    // TODO: Start JACK transport if available
    
    ariel_transport_update_ui(window);
}

void
ariel_transport_update_ui(ArielWindow *window)
{
    if (!window) return;
    
    // Update play button
    if (window->play_button) {
        if (window->is_playing && !window->is_recording) {
            gtk_button_set_icon_name(GTK_BUTTON(window->play_button), "media-playback-pause-symbolic");
            gtk_widget_add_css_class(window->play_button, "suggested-action");
        } else {
            gtk_button_set_icon_name(GTK_BUTTON(window->play_button), "media-playback-start-symbolic");
            gtk_widget_remove_css_class(window->play_button, "suggested-action");
        }
    }
    
    // Update stop button
    if (window->stop_button) {
        if (window->is_playing || window->is_recording) {
            gtk_widget_add_css_class(window->stop_button, "suggested-action");
        } else {
            gtk_widget_remove_css_class(window->stop_button, "suggested-action");
        }
    }
    
    // Update record button
    if (window->record_button) {
        if (window->is_recording) {
            gtk_widget_add_css_class(window->record_button, "destructive-action");
        } else {
            gtk_widget_remove_css_class(window->record_button, "destructive-action");
        }
    }
}

GtkWidget *
ariel_create_transport(ArielWindow *window)
{
    GtkWidget *box;
    
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    // Play button
    window->play_button = gtk_button_new_from_icon_name("media-playback-start-symbolic");
    gtk_widget_add_css_class(window->play_button, "circular");
    g_signal_connect(window->play_button, "clicked", G_CALLBACK(on_play_clicked), window);
    
    // Stop button
    window->stop_button = gtk_button_new_from_icon_name("media-playback-stop-symbolic");
    gtk_widget_add_css_class(window->stop_button, "circular");
    g_signal_connect(window->stop_button, "clicked", G_CALLBACK(on_stop_clicked), window);
    
    // Record button
    window->record_button = gtk_button_new_from_icon_name("media-record-symbolic");
    gtk_widget_add_css_class(window->record_button, "circular");
    g_signal_connect(window->record_button, "clicked", G_CALLBACK(on_record_clicked), window);
    
    gtk_box_append(GTK_BOX(box), window->play_button);
    gtk_box_append(GTK_BOX(box), window->stop_button);
    gtk_box_append(GTK_BOX(box), window->record_button);
    
    // Initialize transport state
    window->is_playing = FALSE;
    window->is_recording = FALSE;
    ariel_transport_update_ui(window);
    
    return box;
}