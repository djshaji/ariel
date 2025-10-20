#include "ariel.h"

// Forward declarations
static void on_remove_all_clicked(GtkButton *button, ArielWindow *window);
static void on_remove_plugin_clicked(GtkButton *button, ArielWindow *window);
static void on_bypass_toggled(GtkToggleButton *button, ArielActivePlugin *plugin);
static gboolean on_plugin_drop(GtkDropTarget *target, const GValue *value, double x, double y, ArielWindow *window);
static GdkDragAction on_drop_enter(GtkDropTarget *target, double x, double y, GtkWidget *plugins_box);
static void on_drop_leave(GtkDropTarget *target, GtkWidget *plugins_box);

// Callback for Remove All button
static void
on_remove_all_clicked(G_GNUC_UNUSED GtkButton *button, ArielWindow *window)
{
    if (!window) {
        g_warning("No window reference for remove all button");
        return;
    }
    
    // Get plugin manager and clear all active plugins
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager || !manager->active_plugin_store) {
        return;
    }
    
    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store));
    
    if (n_active == 0) {
        return; // Nothing to remove
    }
    
    // Deactivate and remove all active plugins
    while (g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store)) > 0) {
        ArielActivePlugin *plugin = g_list_model_get_item(G_LIST_MODEL(manager->active_plugin_store), 0);
        if (plugin) {
            // Deactivate plugin processing
            ariel_active_plugin_deactivate(plugin);
            g_object_unref(plugin);
        }
        
        // Remove from store
        g_list_store_remove(manager->active_plugin_store, 0);
    }
    
    // Remove all items at once to be safe
    g_list_store_remove_all(manager->active_plugin_store);
    
    // Update the UI
    ariel_update_active_plugins_view(window);
}

// Callback for individual plugin remove buttons
static void
on_remove_plugin_clicked(G_GNUC_UNUSED GtkButton *button, ArielWindow *window)
{
    if (!window) {
        return;
    }
    
    // Get the active plugin associated with this button
    ArielActivePlugin *plugin = g_object_get_data(G_OBJECT(button), "active-plugin");
    if (!plugin) {
        return;
    }
    
    // Get plugin manager
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager || !manager->active_plugin_store) {
        return;
    }
    
    // Find and remove the plugin from the store
    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store));
    for (guint i = 0; i < n_active; i++) {
        ArielActivePlugin *store_plugin = g_list_model_get_item(G_LIST_MODEL(manager->active_plugin_store), i);
        if (store_plugin == plugin) {
            // Deactivate plugin processing
            ariel_active_plugin_deactivate(plugin);
            g_list_store_remove(manager->active_plugin_store, i);
            g_object_unref(store_plugin);
            break;
        }
        g_object_unref(store_plugin);
    }
    
    // Update the UI
    ariel_update_active_plugins_view(window);
}

// Callback for bypass button toggle
static void
on_bypass_toggled(GtkToggleButton *button, ArielActivePlugin *plugin)
{
    if (!plugin) {
        return;
    }
    
    gboolean bypass = gtk_toggle_button_get_active(button);
    ariel_active_plugin_set_bypass(plugin, bypass);
}

// Drop target callbacks
static gboolean
on_plugin_drop(G_GNUC_UNUSED GtkDropTarget *target, const GValue *value, G_GNUC_UNUSED double x, G_GNUC_UNUSED double y, ArielWindow *window)
{
    if (!G_VALUE_HOLDS_STRING(value)) {
        return FALSE;
    }
    
    const char *plugin_uri = g_value_get_string(value);
    if (!plugin_uri) {
        return FALSE;
    }
    
    // Find the plugin by URI in the plugin manager
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    if (!manager || !engine || !engine->active) {
        g_warning("Cannot load plugin via drag & drop - audio engine not running");
        return FALSE;
    }
    
    // Search for plugin with matching URI
    guint n_plugins = g_list_model_get_n_items(G_LIST_MODEL(manager->plugin_store));
    for (guint i = 0; i < n_plugins; i++) {
        ArielPluginInfo *plugin_info = g_list_model_get_item(G_LIST_MODEL(manager->plugin_store), i);
        if (plugin_info && strcmp(ariel_plugin_info_get_uri(plugin_info), plugin_uri) == 0) {
            // Load the plugin
            ArielActivePlugin *active_plugin = ariel_plugin_manager_load_plugin(manager, plugin_info, engine);
            if (active_plugin) {
                g_print("Successfully loaded plugin via drag & drop: %s\n", 
                        ariel_active_plugin_get_name(active_plugin));
                
                // Update the active plugins view
                ariel_update_active_plugins_view(window);
                
                g_object_unref(active_plugin);
                g_object_unref(plugin_info);
                return TRUE;
            }
            g_object_unref(plugin_info);
            break;
        }
        g_object_unref(plugin_info);
    }
    
    g_warning("Could not find plugin with URI: %s", plugin_uri);
    return FALSE;
}

static GdkDragAction
on_drop_enter(G_GNUC_UNUSED GtkDropTarget *target, G_GNUC_UNUSED double x, G_GNUC_UNUSED double y, GtkWidget *plugins_box)
{
    // Add visual feedback for drop zone
    gtk_widget_add_css_class(plugins_box, "drop-target");
    return GDK_ACTION_COPY;
}

static void
on_drop_leave(G_GNUC_UNUSED GtkDropTarget *target, GtkWidget *plugins_box)
{
    // Remove visual feedback
    gtk_widget_remove_css_class(plugins_box, "drop-target");
}

// Create active plugins view
GtkWidget *
ariel_create_active_plugins_view(ArielWindow *window)
{
    // Create scrolled window
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);
    
    // Create header box with title and remove all button
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    GtkWidget *header = gtk_label_new("Active Plugins");
    gtk_widget_add_css_class(header, "title-2");
    gtk_label_set_xalign(GTK_LABEL(header), 0.0);
    gtk_widget_set_hexpand(header, TRUE);
    gtk_box_append(GTK_BOX(header_box), header);
    
    // Remove All button
    GtkWidget *remove_all_btn = gtk_button_new_with_label("Remove All");
    gtk_widget_add_css_class(remove_all_btn, "destructive-action");
    gtk_widget_add_css_class(remove_all_btn, "pill");
    g_signal_connect(remove_all_btn, "clicked", 
                     G_CALLBACK(on_remove_all_clicked), window);
    gtk_box_append(GTK_BOX(header_box), remove_all_btn);
    
    gtk_box_append(GTK_BOX(main_box), header_box);
    
    // Create separator
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(main_box), separator);
    
    // Create container for active plugins
    GtkWidget *plugins_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_append(GTK_BOX(main_box), plugins_box);
    gtk_widget_set_vexpand(plugins_box, TRUE);
    gtk_widget_set_hexpand(plugins_box, TRUE);
    
    // Store reference to plugins container for updates
    g_object_set_data(G_OBJECT(scrolled), "plugins-box", plugins_box);
    // Store window reference for plugin removal callbacks
    g_object_set_data(G_OBJECT(plugins_box), "window", window);
    
    // Set up drop target for the plugins box
    GtkDropTarget *drop_target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_plugin_drop), window);
    g_signal_connect(drop_target, "enter", G_CALLBACK(on_drop_enter), plugins_box);
    g_signal_connect(drop_target, "leave", G_CALLBACK(on_drop_leave), plugins_box);
    gtk_widget_add_controller(plugins_box, GTK_EVENT_CONTROLLER(drop_target));
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), main_box);
    
    return scrolled;
}

// Update active plugins view  
void
ariel_update_active_plugins_view(ArielWindow *window)
{
    if (!window) return;
    
    // Find the active plugins container in the window
    GtkWidget *active_plugins_view = window->active_plugins;
    if (!active_plugins_view) return;
    
    GtkWidget *plugins_box = g_object_get_data(G_OBJECT(active_plugins_view), "plugins-box");
    if (!plugins_box) return;
    
    // Clear existing plugins
    GtkWidget *child = gtk_widget_get_first_child(plugins_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(plugins_box), child);
        child = next;
    }
    
    // Get active plugins from plugin manager
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager || !manager->active_plugin_store) {
        g_warning("Plugin manager or active plugin store not available");
        return;
    }
    
    // Defensive check to ensure active_plugin_store is a valid GListModel
    if (!G_IS_LIST_MODEL(manager->active_plugin_store)) {
        g_warning("Active plugin store is not a valid GListModel");
        return;
    }
    
    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store));
    
    if (n_active == 0) {
        // Show "no active plugins" message
        GtkWidget *empty_label = gtk_label_new("No active plugins\nClick on a plugin in the left panel to load it");
        gtk_label_set_justify(GTK_LABEL(empty_label), GTK_JUSTIFY_CENTER);
        gtk_widget_add_css_class(empty_label, "dim-label");
        gtk_widget_set_vexpand(empty_label, TRUE);
        gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(plugins_box), empty_label);
    } else {
        // Add widgets for each active plugin
        for (guint i = 0; i < n_active; i++) {
            ArielActivePlugin *plugin = g_list_model_get_item(G_LIST_MODEL(manager->active_plugin_store), i);
            if (plugin) {
                GtkWidget *plugin_widget = ariel_create_active_plugin_widget(plugin, window);
                if (plugin_widget) {
                    gtk_box_append(GTK_BOX(plugins_box), plugin_widget);
                }
                g_object_unref(plugin);
            }
        }
    }
}

// Create widget for a single active plugin
GtkWidget *
ariel_create_active_plugin_widget(ArielActivePlugin *plugin, ArielWindow *window)
{
    if (!plugin) return NULL;
    
    // Create main frame
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "card");
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);
    
    // Create header box with plugin name and controls
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    // Plugin name
    const char *plugin_name = ariel_active_plugin_get_name(plugin);
    GtkWidget *name_label = gtk_label_new(plugin_name);
    gtk_widget_add_css_class(name_label, "title-4");
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
    gtk_widget_set_hexpand(name_label, TRUE);
    gtk_box_append(GTK_BOX(header_box), name_label);
    
    // Bypass button
    GtkWidget *bypass_btn = gtk_toggle_button_new_with_label("Bypass");
    gtk_widget_add_css_class(bypass_btn, "pill");
    
    // Set initial state based on plugin bypass status
    gboolean is_bypassed = ariel_active_plugin_get_bypass(plugin);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bypass_btn), is_bypassed);
    
    // Connect callback
    g_signal_connect(bypass_btn, "toggled", 
                     G_CALLBACK(on_bypass_toggled), plugin);
    
    gtk_box_append(GTK_BOX(header_box), bypass_btn);
    
    // Remove button
    GtkWidget *remove_btn = gtk_button_new_with_label("Remove");
    gtk_widget_add_css_class(remove_btn, "destructive-action");
    gtk_widget_add_css_class(remove_btn, "pill");
    g_object_set_data(G_OBJECT(remove_btn), "active-plugin", plugin);
    g_signal_connect(remove_btn, "clicked", 
                     G_CALLBACK(on_remove_plugin_clicked), window);
    gtk_box_append(GTK_BOX(header_box), remove_btn);
    
    gtk_box_append(GTK_BOX(main_box), header_box);
    
    // Create parameter controls
    GtkWidget *params_widget = ariel_create_parameter_controls(plugin);
    if (params_widget) {
        gtk_box_append(GTK_BOX(main_box), params_widget);
    }
    
    gtk_frame_set_child(GTK_FRAME(frame), main_box);
    
    return frame;
}