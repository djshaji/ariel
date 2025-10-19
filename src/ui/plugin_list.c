#include "ariel.h"

static void
on_plugin_row_activated(GtkListView *list_view, guint position, ArielWindow *window)
{
    // Get the plugin info from the selection
    ArielPluginManager *plugin_manager = ariel_app_get_plugin_manager(window->app);
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    ArielPluginInfo *plugin_info = g_list_model_get_item(
        G_LIST_MODEL(plugin_manager->plugin_store), position);
    
    if (!plugin_info) {
        g_warning("No plugin found at position %u", position);
        return;
    }
    
    // Check if audio engine is running
    if (!engine->active) {
        g_warning("Cannot load plugin - audio engine is not running");
        g_object_unref(plugin_info);
        return;
    }
    
    // Load the plugin
    ArielActivePlugin *active_plugin = ariel_plugin_manager_load_plugin(
        plugin_manager, plugin_info, engine);
    
    if (active_plugin) {
        g_print("Successfully loaded plugin: %s\n", 
                ariel_active_plugin_get_name(active_plugin));
        g_object_unref(active_plugin); // List store holds its own reference
    } else {
        g_warning("Failed to load plugin: %s", 
                  ariel_plugin_info_get_name(plugin_info));
    }
    
    g_object_unref(plugin_info);
}

GtkWidget *
ariel_create_plugin_list(ArielWindow *window)
{
    GtkWidget *scrolled;
    GtkWidget *list_view;
    GtkListItemFactory *factory;
    GtkSelectionModel *selection_model;
    
    // Create scrolled window
    scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, 300, -1);
    
    // Create list item factory
    factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_plugin_list_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_plugin_list_item), NULL);
    
    // Create selection model
    ArielPluginManager *plugin_manager = ariel_app_get_plugin_manager(window->app);
    selection_model = GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(plugin_manager->plugin_store))
    );
    
    // Create list view
    list_view = gtk_list_view_new(selection_model, factory);
    g_signal_connect(list_view, "activate",
                     G_CALLBACK(on_plugin_row_activated), window);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_view);
    
    return scrolled;
}

void
setup_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
    GtkWidget *box;
    GtkWidget *name_label;
    GtkWidget *author_label;
    
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);
    
    name_label = gtk_label_new("");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(name_label, "heading");
    
    author_label = gtk_label_new("");
    gtk_widget_set_halign(author_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(author_label, "dim-label");
    
    gtk_box_append(GTK_BOX(box), name_label);
    gtk_box_append(GTK_BOX(box), author_label);
    
    gtk_list_item_set_child(list_item, box);
}

void
bind_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
    ArielPluginInfo *plugin_info = gtk_list_item_get_item(list_item);
    if (!plugin_info || !ARIEL_IS_PLUGIN_INFO(plugin_info)) {
        return;
    }
    
    GtkWidget *box = gtk_list_item_get_child(list_item);
    GtkWidget *name_label = gtk_widget_get_first_child(box);
    GtkWidget *author_label = gtk_widget_get_last_child(box);
    
    // Set actual plugin data
    gtk_label_set_text(GTK_LABEL(name_label), ariel_plugin_info_get_name(plugin_info));
    gtk_label_set_text(GTK_LABEL(author_label), ariel_plugin_info_get_author(plugin_info));
}

GtkWidget *
ariel_create_transport(ArielWindow *window)
{
    GtkWidget *box;
    GtkWidget *play_button;
    GtkWidget *stop_button;
    GtkWidget *record_button;
    
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    // Play button
    play_button = gtk_button_new_from_icon_name("media-playback-start-symbolic");
    gtk_widget_add_css_class(play_button, "circular");
    gtk_widget_add_css_class(play_button, "suggested-action");
    
    // Stop button
    stop_button = gtk_button_new_from_icon_name("media-playback-stop-symbolic");
    gtk_widget_add_css_class(stop_button, "circular");
    
    // Record button
    record_button = gtk_button_new_from_icon_name("media-record-symbolic");
    gtk_widget_add_css_class(record_button, "circular");
    gtk_widget_add_css_class(record_button, "destructive-action");
    
    gtk_box_append(GTK_BOX(box), play_button);
    gtk_box_append(GTK_BOX(box), stop_button);
    gtk_box_append(GTK_BOX(box), record_button);
    
    return box;
}