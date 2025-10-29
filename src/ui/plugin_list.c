#include "ariel.h"

// Forward declarations
static GdkContentProvider *on_drag_prepare(GtkDragSource *source, double x, double y, gpointer user_data);
static void on_drag_begin(GtkDragSource *source, GdkDrag *drag, gpointer user_data);
static void on_search_changed(GtkSearchEntry *entry, gpointer user_data);
static void on_category_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data);
static gboolean plugin_filter_func(gpointer item, gpointer user_data);
static void populate_category_dropdown(GtkDropDown *dropdown, ArielPluginManager *manager);

static void
on_plugin_row_activated(GtkListView *list_view, guint position, ArielWindow *window)
{
    // Get the selection model from the list view
    GtkSelectionModel *selection_model = gtk_list_view_get_model(list_view);
    if (!selection_model) {
        g_warning("No selection model found");
        return;
    }
    
    // Get the plugin info from the filtered model
    ArielPluginInfo *plugin_info = g_list_model_get_item(
        G_LIST_MODEL(selection_model), position);
    
    if (!plugin_info) {
        g_warning("No plugin found at position %u", position);
        return;
    }
    
    ArielPluginManager *plugin_manager = ariel_app_get_plugin_manager(window->app);
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    
    if (!plugin_manager) {
        g_warning("Cannot load plugin - plugin manager not available");
        g_object_unref(plugin_info);
        return;
    }
    
    if (!engine) {
        g_warning("Cannot load plugin - audio engine not available");
        g_object_unref(plugin_info);
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
        
        // Update the active plugins view
        ariel_update_active_plugins_view(window);
        
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
    GtkWidget *main_box;
    GtkWidget *search_entry;
    GtkWidget *scrolled;
    GtkWidget *list_view;
    GtkListItemFactory *factory;
    GtkSelectionModel *selection_model;
    GtkCustomFilter *custom_filter;
    GtkFilterListModel *filter_model;
    
    // Create main container
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);
    
    // Create header with title
    GtkWidget *header_label = gtk_label_new("LV2 Plugins");
    gtk_widget_add_css_class(header_label, "title-2");
    gtk_label_set_xalign(GTK_LABEL(header_label), 0.0);
    gtk_box_append(GTK_BOX(main_box), header_label);
    
    // Create filter controls container
    GtkWidget *filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    // Create search entry
    search_entry = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(search_entry), "Search plugins...");
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_box_append(GTK_BOX(filter_box), search_entry);
    
    // Create category dropdown
    GtkWidget *category_dropdown = gtk_drop_down_new(NULL, NULL);
    gtk_widget_set_size_request(category_dropdown, 150, -1);
    ArielPluginManager *plugin_manager = ariel_app_get_plugin_manager(window->app);
    if (plugin_manager) {
        populate_category_dropdown(GTK_DROP_DOWN(category_dropdown), plugin_manager);
    } else {
        g_warning("Plugin manager not available - skipping category dropdown population");
        // Add a placeholder entry
        gtk_drop_down_set_model(GTK_DROP_DOWN(category_dropdown), NULL);
    }
    gtk_box_append(GTK_BOX(filter_box), category_dropdown);
    
    gtk_box_append(GTK_BOX(main_box), filter_box);
    
    // Create scrolled window
    scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, 300, -1);
    gtk_widget_set_vexpand(scrolled, TRUE);
    
    // Create list item factory
    factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_plugin_list_item), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_plugin_list_item), NULL);
    
    // Create filter for search functionality
    // Create filter data structure to pass both widgets
    GtkWidget **filter_data = g_new(GtkWidget*, 2);
    filter_data[0] = search_entry;
    filter_data[1] = category_dropdown;
    
    custom_filter = gtk_custom_filter_new(plugin_filter_func, filter_data, g_free);
    filter_model = gtk_filter_list_model_new(G_LIST_MODEL(plugin_manager->plugin_store), GTK_FILTER(custom_filter));
    
    // Create selection model with filtered model
    selection_model = GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(filter_model))
    );
    
    // Create list view
    list_view = gtk_list_view_new(selection_model, factory);
    g_signal_connect(list_view, "activate",
                     G_CALLBACK(on_plugin_row_activated), window);
    
    // Connect search and category functionality
    g_object_set_data(G_OBJECT(search_entry), "filter", custom_filter);
    g_object_set_data(G_OBJECT(category_dropdown), "filter", custom_filter);
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), custom_filter);
    g_signal_connect(category_dropdown, "notify::selected", G_CALLBACK(on_category_changed), custom_filter);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_view);
    gtk_box_append(GTK_BOX(main_box), scrolled);
    
    return main_box;
}

// Drag source callbacks
static GdkContentProvider *
on_drag_prepare(G_GNUC_UNUSED GtkDragSource *source, G_GNUC_UNUSED double x, G_GNUC_UNUSED double y, gpointer user_data)
{
    ArielPluginInfo *plugin_info = ARIEL_PLUGIN_INFO(user_data);
    if (!plugin_info) return NULL;
    
    // Get the plugin URI to transfer
    const char *plugin_uri = ariel_plugin_info_get_uri(plugin_info);
    if (!plugin_uri) return NULL;
    
    g_print("Preparing to drag plugin: %s\n", plugin_uri);
    
    // Create content provider with the plugin URI
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_value_set_string(&value, plugin_uri);
    
    return gdk_content_provider_new_for_value(&value);
}

static void
on_drag_begin(G_GNUC_UNUSED GtkDragSource *source, G_GNUC_UNUSED GdkDrag *drag, gpointer user_data)
{
    ArielPluginInfo *plugin_info = ARIEL_PLUGIN_INFO(user_data);
    if (!plugin_info) return;
    
    const char *plugin_name = ariel_plugin_info_get_name(plugin_info);
    g_print("Started dragging plugin: %s\n", plugin_name);
    
    // TODO: Could create a custom drag icon here if desired
}

void
setup_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
    GtkWidget *box;
    GtkWidget *name_label;
    GtkWidget *author_label;
    
    // Create a vertical box to hold plugin info
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    
    // Create labels for plugin name and author
    name_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
    gtk_widget_add_css_class(name_label, "title-4");
    
    author_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(author_label), 0.0);
    gtk_widget_add_css_class(author_label, "caption");
    gtk_widget_add_css_class(author_label, "dim-label");
    
    gtk_box_append(GTK_BOX(box), name_label);
    gtk_box_append(GTK_BOX(box), author_label);
    
    // Store label references for binding
    g_object_set_data(G_OBJECT(box), "name-label", name_label);
    g_object_set_data(G_OBJECT(box), "author-label", author_label);
    
    // Set up drag source for plugin items
    GtkDragSource *drag_source = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag_source, GDK_ACTION_COPY);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(drag_source));
    
    // Store drag source reference for binding
    g_object_set_data(G_OBJECT(box), "drag-source", drag_source);
    
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
    
    // Connect drag callbacks with plugin data
    GtkDragSource *drag_source = g_object_get_data(G_OBJECT(box), "drag-source");
    if (drag_source) {
        g_signal_connect(drag_source, "prepare", G_CALLBACK(on_drag_prepare), plugin_info);
        g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), plugin_info);
    }
}

// Search functionality
static void
on_search_changed(G_GNUC_UNUSED GtkSearchEntry *entry, gpointer user_data)
{
    GtkCustomFilter *filter = GTK_CUSTOM_FILTER(user_data);
    
    // Trigger filter update
    gtk_filter_changed(GTK_FILTER(filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void
on_category_changed(G_GNUC_UNUSED GtkDropDown *dropdown, G_GNUC_UNUSED GParamSpec *pspec, gpointer user_data)
{
    GtkCustomFilter *filter = GTK_CUSTOM_FILTER(user_data);
    
    // Trigger filter update
    gtk_filter_changed(GTK_FILTER(filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void
populate_category_dropdown(GtkDropDown *dropdown, ArielPluginManager *manager)
{
    if (!dropdown || !manager) {
        g_warning("Invalid arguments to populate_category_dropdown");
        return;
    }

    GtkStringList *category_list = gtk_string_list_new(NULL);
    GHashTable *categories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    
    // Add "All Categories" option
    gtk_string_list_append(category_list, "All Categories");
    
    // Collect unique categories from all plugins
    guint n_plugins = g_list_model_get_n_items(G_LIST_MODEL(manager->plugin_store));
    for (guint i = 0; i < n_plugins; i++) {
        ArielPluginInfo *info = g_list_model_get_item(G_LIST_MODEL(manager->plugin_store), i);
        if (info) {
            const char *category = ariel_plugin_info_get_category(info);
            if (category && !g_hash_table_contains(categories, category)) {
                g_hash_table_insert(categories, g_strdup(category), GINT_TO_POINTER(1));
                gtk_string_list_append(category_list, category);
            }
            g_object_unref(info);
        }
    }
    
    // Set the model
    gtk_drop_down_set_model(dropdown, G_LIST_MODEL(category_list));
    gtk_drop_down_set_selected(dropdown, 0); // Select "All Categories"
    
    g_hash_table_destroy(categories);
}

static gboolean
plugin_filter_func(gpointer item, gpointer user_data)
{
    ArielPluginInfo *plugin_info = ARIEL_PLUGIN_INFO(item);
    GtkWidget **filter_data = (GtkWidget**)user_data;
    
    if (!plugin_info || !filter_data) {
        return TRUE; // Show all if no filter context
    }
    
    GtkSearchEntry *search_entry = GTK_SEARCH_ENTRY(filter_data[0]);
    GtkDropDown *category_dropdown = GTK_DROP_DOWN(filter_data[1]);
    
    // Check category filter first
    guint selected_category = gtk_drop_down_get_selected(category_dropdown);
    if (selected_category > 0) { // 0 is "All Categories"
        GtkStringList *category_list = GTK_STRING_LIST(gtk_drop_down_get_model(category_dropdown));
        GtkStringObject *selected_item = g_list_model_get_item(G_LIST_MODEL(category_list), selected_category);
        if (selected_item) {
            const char *selected_category_name = gtk_string_object_get_string(selected_item);
            const char *plugin_category = ariel_plugin_info_get_category(plugin_info);
            
            if (!plugin_category || strcmp(plugin_category, selected_category_name) != 0) {
                g_object_unref(selected_item);
                return FALSE; // Category doesn't match
            }
            g_object_unref(selected_item);
        }
    }
    
    // Check search filter
    const char *search_text = gtk_editable_get_text(GTK_EDITABLE(search_entry));
    
    // If search is empty, show all plugins (that pass category filter)
    if (!search_text || strlen(search_text) == 0) {
        return TRUE;
    }
    
    // Convert search text to lowercase for case-insensitive search
    char *search_lower = g_utf8_strdown(search_text, -1);
    
    gboolean match = FALSE;
    
    // Search in plugin name
    const char *plugin_name = ariel_plugin_info_get_name(plugin_info);
    if (plugin_name) {
        char *name_lower = g_utf8_strdown(plugin_name, -1);
        if (strstr(name_lower, search_lower) != NULL) {
            match = TRUE;
        }
        g_free(name_lower);
    }
    
    // Search in author name if name didn't match
    if (!match) {
        const char *author = ariel_plugin_info_get_author(plugin_info);
        if (author) {
            char *author_lower = g_utf8_strdown(author, -1);
            if (strstr(author_lower, search_lower) != NULL) {
                match = TRUE;
            }
            g_free(author_lower);
        }
    }
    
    // Search in category if still no match
    if (!match) {
        const char *category = ariel_plugin_info_get_category(plugin_info);
        if (category) {
            char *category_lower = g_utf8_strdown(category, -1);
            if (strstr(category_lower, search_lower) != NULL) {
                match = TRUE;
            }
            g_free(category_lower);
        }
    }
    
    // Search in URI if still no match
    if (!match) {
        const char *uri = ariel_plugin_info_get_uri(plugin_info);
        if (uri) {
            char *uri_lower = g_utf8_strdown(uri, -1);
            if (strstr(uri_lower, search_lower) != NULL) {
                match = TRUE;
            }
            g_free(uri_lower);
        }
    }
    
    g_free(search_lower);
    return match;
}

