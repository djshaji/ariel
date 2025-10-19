#include "ariel.h"

GtkWidget *
ariel_create_mixer(ArielWindow *window)
{
    GtkWidget *scrolled;
    GtkWidget *box;
    GtkWidget *label;
    
    // Create scrolled window
    scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_size_request(scrolled, -1, 200);
    
    // Create horizontal box for mixer channels
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    
    // Add placeholder label
    label = gtk_label_new("Mixer channels will appear here");
    gtk_widget_add_css_class(label, "dim-label");
    gtk_box_append(GTK_BOX(box), label);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    return scrolled;
}

GtkWidget *
ariel_create_mixer_channel(const char *name, int channel_id)
{
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *volume_scale;
    GtkWidget *pan_scale;
    GtkWidget *mute_button;
    
    // Create frame for the channel
    frame = gtk_frame_new(name);
    gtk_widget_set_size_request(frame, 80, -1);
    
    // Create vertical box for controls
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(vbox, 4);
    gtk_widget_set_margin_end(vbox, 4);
    gtk_widget_set_margin_top(vbox, 4);
    gtk_widget_set_margin_bottom(vbox, 4);
    
    // Volume control (vertical slider)
    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(volume_scale), 0.8);
    gtk_scale_set_draw_value(GTK_SCALE(volume_scale), FALSE);
    gtk_widget_set_vexpand(volume_scale, TRUE);
    gtk_range_set_inverted(GTK_RANGE(volume_scale), TRUE);
    
    // Pan control (horizontal slider)
    pan_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -1.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(pan_scale), 0.0);
    gtk_scale_set_draw_value(GTK_SCALE(pan_scale), FALSE);
    gtk_widget_set_size_request(pan_scale, 60, -1);
    
    // Mute button
    mute_button = gtk_toggle_button_new_with_label("M");
    gtk_widget_add_css_class(mute_button, "destructive-action");
    
    // Volume label
    label = gtk_label_new("Vol");
    gtk_widget_add_css_class(label, "caption");
    
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), volume_scale);
    
    label = gtk_label_new("Pan");
    gtk_widget_add_css_class(label, "caption");
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), pan_scale);
    gtk_box_append(GTK_BOX(vbox), mute_button);
    
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    
    return frame;
}