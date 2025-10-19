#include "ariel.h"

// Forward declarations
static GdkContentProvider *on_drag_prepare(GtkDragSource *source, double x, double y, gpointer user_data);
static void on_drag_begin(GtkDragSource *source, GdkDrag *drag, gpointer user_data);

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

