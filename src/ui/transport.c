#include "ariel.h"

static void
on_audio_toggle_clicked(GtkToggleButton *button, ArielWindow *window)
{
    gboolean active = gtk_toggle_button_get_active(button);
    
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    if (active) {
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
                                    gtk_label_new("Ariel LV2 Host"));
    
    // Audio engine toggle button
    window->audio_toggle = gtk_toggle_button_new_with_label("Audio: OFF");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), window->audio_toggle);
    g_signal_connect(window->audio_toggle, "clicked",
                     G_CALLBACK(on_audio_toggle_clicked), window);
    
    // Menu button
    menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), menu_button);
    
    return header_bar;
}