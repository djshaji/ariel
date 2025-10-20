#include "ariel.h"

// Forward declarations
static void on_remove_all_clicked(GtkButton *button, ArielWindow *window);
static void on_remove_plugin_clicked(GtkButton *button, ArielWindow *window);
static void on_bypass_toggled(GtkToggleButton *button, ArielActivePlugin *plugin);
static void on_save_preset_clicked(GtkButton *button, gpointer user_data);
static void on_load_preset_clicked(GtkButton *button, gpointer user_data);
static void on_save_preset_ok(GtkButton *button, GtkEntry *entry);
static void on_load_preset_ok(GtkButton *button, GtkDropDown *dropdown);
static void on_save_chain_preset_clicked(GtkButton *button, ArielWindow *window);
static void on_load_chain_preset_clicked(GtkButton *button, ArielWindow *window);
static void on_save_chain_preset_ok(GtkButton *button, gpointer user_data);
static void on_load_chain_preset_ok(GtkButton *button, gpointer user_data);
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

// Callback for save preset button
static void
on_save_preset_clicked(G_GNUC_UNUSED GtkButton *button, gpointer user_data)
{
    ArielActivePlugin *plugin = (ArielActivePlugin *)user_data;
    if (!plugin) return;
    
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Save Preset");
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 350, 150);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    
    GtkWidget *label = gtk_label_new("Preset name:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_append(GTK_BOX(box), label);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter preset name");
    gtk_box_append(GTK_BOX(box), entry);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    gtk_box_append(GTK_BOX(button_box), save_btn);
    
    gtk_box_append(GTK_BOX(box), button_box);
    gtk_window_set_child(GTK_WINDOW(window), box);
    
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), window);
    
    g_signal_connect_swapped(save_btn, "clicked", G_CALLBACK(gtk_window_destroy), window);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_preset_ok), entry);
    
    g_object_set_data(G_OBJECT(entry), "plugin", plugin);
    gtk_window_present(GTK_WINDOW(window));
}

// Helper callback for save preset OK button
static void
on_save_preset_ok(G_GNUC_UNUSED GtkButton *button, GtkEntry *entry)
{
    ArielActivePlugin *plugin = g_object_get_data(G_OBJECT(entry), "plugin");
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (text && strlen(text) > 0) {
        ArielConfig *config = ariel_config_new();
        const char *config_dir = ariel_config_get_dir(config);
        char *preset_dir = g_build_filename(config_dir, "presets", NULL);
        
        ariel_active_plugin_save_preset(plugin, text, preset_dir);
        
        g_free(preset_dir);
        ariel_config_free(config);
    }
}

// Callback for load preset button
static void
on_load_preset_clicked(G_GNUC_UNUSED GtkButton *button, gpointer user_data)
{
    ArielActivePlugin *plugin = (ArielActivePlugin *)user_data;
    if (!plugin) return;
    
    // Get config directory for presets
    ArielConfig *config = ariel_config_new();
    const char *config_dir = ariel_config_get_dir(config);
    char *preset_dir = g_build_filename(config_dir, "presets", NULL);
    
    // Get list of available presets for this plugin
    char **preset_list = ariel_active_plugin_list_presets(plugin, preset_dir);
    
    if (!preset_list || !preset_list[0]) {
        g_print("No presets found for plugin %s\n", ariel_active_plugin_get_name(plugin));
        
        if (preset_list) ariel_active_plugin_free_preset_list(preset_list);
        g_free(preset_dir);
        ariel_config_free(config);
        return;
    }
    
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Load Preset");
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 350, 200);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    
    GtkWidget *label = gtk_label_new("Select preset:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_append(GTK_BOX(box), label);
    
    // Create dropdown using GtkDropDown
    GtkStringList *string_list = gtk_string_list_new(NULL);
    for (int i = 0; preset_list[i] != NULL; i++) {
        gtk_string_list_append(string_list, preset_list[i]);
    }
    
    GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(string_list), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), 0);
    gtk_box_append(GTK_BOX(box), dropdown);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    
    GtkWidget *load_btn = gtk_button_new_with_label("Load");
    gtk_widget_add_css_class(load_btn, "suggested-action");
    gtk_box_append(GTK_BOX(button_box), load_btn);
    
    gtk_box_append(GTK_BOX(box), button_box);
    gtk_window_set_child(GTK_WINDOW(window), box);
    
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), window);
    
    g_signal_connect_swapped(load_btn, "clicked", G_CALLBACK(gtk_window_destroy), window);
    g_signal_connect(load_btn, "clicked", G_CALLBACK(on_load_preset_ok), dropdown);
    
    g_object_set_data_full(G_OBJECT(dropdown), "preset-list", preset_list, 
                          (GDestroyNotify)ariel_active_plugin_free_preset_list);
    g_object_set_data_full(G_OBJECT(dropdown), "preset-dir", preset_dir, g_free);
    g_object_set_data(G_OBJECT(dropdown), "plugin", plugin);
    
    gtk_window_present(GTK_WINDOW(window));
    ariel_config_free(config);
}

// Helper callback for load preset OK button
static void
on_load_preset_ok(G_GNUC_UNUSED GtkButton *button, GtkDropDown *dropdown)
{
    char **preset_list = g_object_get_data(G_OBJECT(dropdown), "preset-list");
    char *preset_dir = g_object_get_data(G_OBJECT(dropdown), "preset-dir");
    ArielActivePlugin *plugin = g_object_get_data(G_OBJECT(dropdown), "plugin");
    
    guint selected = gtk_drop_down_get_selected(dropdown);
    if (preset_list && preset_list[selected]) {
        char *preset_path = g_build_filename(preset_dir, preset_list[selected], NULL);
        char *full_preset_path = g_strconcat(preset_path, ".preset", NULL);
        
        ariel_active_plugin_load_preset(plugin, full_preset_path);
        
        g_free(preset_path);
        g_free(full_preset_path);
    }
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
    
    // Create header box with title and buttons
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    GtkWidget *header = gtk_label_new("Active Plugins");
    gtk_widget_add_css_class(header, "title-2");
    gtk_label_set_xalign(GTK_LABEL(header), 0.0);
    gtk_widget_set_hexpand(header, TRUE);
    gtk_box_append(GTK_BOX(header_box), header);
    
    // Save Chain Preset button
    GtkWidget *save_chain_btn = gtk_button_new_with_label("Save Chain");
    gtk_widget_add_css_class(save_chain_btn, "suggested-action");
    gtk_widget_add_css_class(save_chain_btn, "pill");
    gtk_widget_set_tooltip_text(save_chain_btn, "Save current plugin chain as preset");
    g_signal_connect(save_chain_btn, "clicked", 
                     G_CALLBACK(on_save_chain_preset_clicked), window);
    gtk_box_append(GTK_BOX(header_box), save_chain_btn);
    
    // Load Chain Preset button
    GtkWidget *load_chain_btn = gtk_button_new_with_label("Load Chain");
    gtk_widget_add_css_class(load_chain_btn, "pill");
    gtk_widget_set_tooltip_text(load_chain_btn, "Load saved plugin chain preset");
    g_signal_connect(load_chain_btn, "clicked", 
                     G_CALLBACK(on_load_chain_preset_clicked), window);
    gtk_box_append(GTK_BOX(header_box), load_chain_btn);
    
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
    gtk_widget_set_name (frame, "active-plugin-box");
    gtk_widget_set_hexpand(frame, TRUE);
    gtk_widget_set_vexpand(frame, TRUE);
    
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
    
    // Save Preset button
    GtkWidget *save_preset_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_preset_btn, "suggested-action");
    gtk_widget_add_css_class(save_preset_btn, "pill");
    gtk_widget_set_tooltip_text(save_preset_btn, "Save current parameters as preset");
    g_signal_connect(save_preset_btn, "clicked", 
                     G_CALLBACK(on_save_preset_clicked), plugin);
    gtk_box_append(GTK_BOX(header_box), save_preset_btn);
    
    // Load Preset button
    GtkWidget *load_preset_btn = gtk_button_new_with_label("Load");
    gtk_widget_add_css_class(load_preset_btn, "pill");
    gtk_widget_set_tooltip_text(load_preset_btn, "Load saved preset");
    g_signal_connect(load_preset_btn, "clicked", 
                     G_CALLBACK(on_load_preset_clicked), plugin);
    gtk_box_append(GTK_BOX(header_box), load_preset_btn);
    
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

// Chain preset save/load callbacks
static void
on_save_chain_preset_clicked(G_GNUC_UNUSED GtkButton *button, ArielWindow *window)
{
    if (!window) return;
    
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager || !manager->active_plugin_store) return;
    
    // Check if there are any active plugins
    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store));
    if (n_active == 0) {
        // Show message that no plugins are active
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Save Chain Preset");
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(box, 24);
        gtk_widget_set_margin_end(box, 24);
        gtk_widget_set_margin_top(box, 24);
        gtk_widget_set_margin_bottom(box, 24);
        
        GtkWidget *label = gtk_label_new("No active plugins to save.\nLoad some plugins first!");
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
        gtk_widget_set_vexpand(label, TRUE);
        gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(box), label);
        
        GtkWidget *close_btn = gtk_button_new_with_label("OK");
        gtk_widget_add_css_class(close_btn, "suggested-action");
        g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(box), close_btn);
        
        gtk_window_set_child(GTK_WINDOW(dialog), box);
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    
    // Create save dialog
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Save Chain Preset");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    
    GtkWidget *label = gtk_label_new("Enter preset name:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "My Chain Preset");
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(box), entry);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_btn, "suggested-action");
    g_object_set_data(G_OBJECT(save_btn), "entry", entry);
    g_object_set_data(G_OBJECT(save_btn), "window", window);
    g_object_set_data(G_OBJECT(save_btn), "dialog", dialog);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_chain_preset_ok), NULL);
    gtk_box_append(GTK_BOX(button_box), save_btn);
    
    gtk_box_append(GTK_BOX(box), button_box);
    
    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void
on_save_chain_preset_ok(GtkButton *button, G_GNUC_UNUSED gpointer user_data)
{
    GtkEntry *entry = g_object_get_data(G_OBJECT(button), "entry");
    ArielWindow *window = g_object_get_data(G_OBJECT(button), "window");
    GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog");
    
    if (!entry || !window) return;
    
    const char *preset_name = gtk_entry_buffer_get_text(gtk_entry_get_buffer(entry));
    if (!preset_name || strlen(preset_name) == 0) {
        return; // No name entered
    }
    
    // Get preset directory from config
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager) return;
    
    ArielConfig *config = ariel_config_new();
    const char *config_dir = ariel_config_get_dir(config);
    char *preset_dir = g_build_filename(config_dir, "chain_presets", NULL);
    
    // Save the chain preset
    gboolean success = ariel_save_plugin_chain_preset(manager, preset_name, preset_dir);
    
    if (success) {
        g_print("Saved chain preset: %s\n", preset_name);
    } else {
        g_warning("Failed to save chain preset: %s", preset_name);
    }
    
    g_free(preset_dir);
    ariel_config_free(config);
    
    if (dialog) {
        gtk_window_destroy(GTK_WINDOW(dialog));
    }
}

static void
on_load_chain_preset_clicked(G_GNUC_UNUSED GtkButton *button, ArielWindow *window)
{
    if (!window) return;
    
    // Get preset directory
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    if (!manager) return;
    
    ArielConfig *config = ariel_config_new();
    const char *config_dir = ariel_config_get_dir(config);
    char *preset_dir = g_build_filename(config_dir, "chain_presets", NULL);
    
    // Get list of available presets
    char **preset_list = ariel_list_plugin_chain_presets(preset_dir);
    if (!preset_list || !preset_list[0]) {
        // No presets available
        GtkWidget *dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Load Chain Preset");
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_start(box, 24);
        gtk_widget_set_margin_end(box, 24);
        gtk_widget_set_margin_top(box, 24);
        gtk_widget_set_margin_bottom(box, 24);
        
        GtkWidget *label = gtk_label_new("No chain presets found.\nSave a chain preset first!");
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
        gtk_widget_set_vexpand(label, TRUE);
        gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(box), label);
        
        GtkWidget *close_btn = gtk_button_new_with_label("OK");
        gtk_widget_add_css_class(close_btn, "suggested-action");
        g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(box), close_btn);
        
        gtk_window_set_child(GTK_WINDOW(dialog), box);
        gtk_window_present(GTK_WINDOW(dialog));
        
        if (preset_list) ariel_free_plugin_chain_preset_list(preset_list);
        g_free(preset_dir);
        ariel_config_free(config);
        return;
    }
    
    // Create load dialog with dropdown
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Load Chain Preset");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);
    
    GtkWidget *label = gtk_label_new("Select chain preset to load:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);
    
    // Create string list for dropdown
    GtkStringList *string_list = gtk_string_list_new(NULL);
    for (int i = 0; preset_list[i] != NULL; i++) {
        gtk_string_list_append(string_list, preset_list[i]);
    }
    
    GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(string_list), NULL);
    gtk_widget_set_hexpand(dropdown, TRUE);
    gtk_box_append(GTK_BOX(box), dropdown);
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    
    GtkWidget *load_btn = gtk_button_new_with_label("Load");
    gtk_widget_add_css_class(load_btn, "suggested-action");
    g_object_set_data(G_OBJECT(load_btn), "dropdown", dropdown);
    g_object_set_data(G_OBJECT(load_btn), "window", window);
    g_object_set_data(G_OBJECT(load_btn), "dialog", dialog);
    g_object_set_data(G_OBJECT(load_btn), "preset_dir", g_strdup(preset_dir));
    g_signal_connect(load_btn, "clicked", G_CALLBACK(on_load_chain_preset_ok), NULL);
    gtk_box_append(GTK_BOX(button_box), load_btn);
    
    gtk_box_append(GTK_BOX(box), button_box);
    
    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
    
    ariel_free_plugin_chain_preset_list(preset_list);
    g_free(preset_dir);
    ariel_config_free(config);
}

static void
on_load_chain_preset_ok(GtkButton *button, G_GNUC_UNUSED gpointer user_data)
{
    GtkDropDown *dropdown = g_object_get_data(G_OBJECT(button), "dropdown");
    ArielWindow *window = g_object_get_data(G_OBJECT(button), "window");
    GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog");
    char *preset_dir = g_object_get_data(G_OBJECT(button), "preset_dir");
    
    if (!dropdown || !window || !preset_dir) return;
    
    // Get selected preset name
    GtkStringObject *selected = gtk_drop_down_get_selected_item(dropdown);
    if (!selected) return;
    
    const char *preset_name = gtk_string_object_get_string(selected);
    if (!preset_name) return;
    
    // Build preset file path
    char *preset_filename = g_strdup_printf("%s.chain", preset_name);
    char *preset_path = g_build_filename(preset_dir, preset_filename, NULL);
    
    // Load the chain preset
    ArielPluginManager *manager = ariel_app_get_plugin_manager(window->app);
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    if (manager && engine) {
        gboolean success = ariel_load_plugin_chain_preset(manager, engine, preset_path);
        
        if (success) {
            g_print("Loaded chain preset: %s\n", preset_name);
            // Update the active plugins view
            ariel_update_active_plugins_view(window);
        } else {
            g_warning("Failed to load chain preset: %s", preset_name);
        }
    }
    
    g_free(preset_filename);
    g_free(preset_path);
    g_free(preset_dir);
    
    if (dialog) {
        gtk_window_destroy(GTK_WINDOW(dialog));
    }
}