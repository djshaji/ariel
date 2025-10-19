#include "ariel.h"

// ArielPluginInfo GObject implementation
struct _ArielPluginInfo {
    GObject parent;
    char *name;
    char *author;
    char *uri;
    const LilvPlugin *plugin;
};

G_DEFINE_FINAL_TYPE(ArielPluginInfo, ariel_plugin_info, G_TYPE_OBJECT)

static void
ariel_plugin_info_finalize(GObject *object)
{
    ArielPluginInfo *info = ARIEL_PLUGIN_INFO(object);
    
    g_free(info->name);
    g_free(info->author);
    g_free(info->uri);
    
    G_OBJECT_CLASS(ariel_plugin_info_parent_class)->finalize(object);
}

static void
ariel_plugin_info_class_init(ArielPluginInfoClass *class)
{
    G_OBJECT_CLASS(class)->finalize = ariel_plugin_info_finalize;
}

static void
ariel_plugin_info_init(ArielPluginInfo *info)
{
    info->name = NULL;
    info->author = NULL;
    info->uri = NULL;
    info->plugin = NULL;
}

ArielPluginInfo *
ariel_plugin_info_new(const LilvPlugin *plugin)
{
    ArielPluginInfo *info = g_object_new(ARIEL_TYPE_PLUGIN_INFO, NULL);
    
    info->plugin = plugin;
    
    // Get plugin name
    LilvNode *name_node = lilv_plugin_get_name(plugin);
    if (name_node) {
        info->name = g_strdup(lilv_node_as_string(name_node));
        lilv_node_free(name_node);
    } else {
        info->name = g_strdup("Unknown Plugin");
    }
    
    // Get plugin author
    LilvNode *author_node = lilv_plugin_get_author_name(plugin);
    if (author_node) {
        info->author = g_strdup(lilv_node_as_string(author_node));
        lilv_node_free(author_node);
    } else {
        info->author = g_strdup("Unknown Author");
    }
    
    // Get plugin URI
    const LilvNode *uri_node = lilv_plugin_get_uri(plugin);
    if (uri_node) {
        info->uri = g_strdup(lilv_node_as_string(uri_node));
    } else {
        info->uri = g_strdup("");
    }
    
    return info;
}

const char *
ariel_plugin_info_get_name(ArielPluginInfo *info)
{
    g_return_val_if_fail(ARIEL_IS_PLUGIN_INFO(info), NULL);
    return info->name;
}

const char *
ariel_plugin_info_get_author(ArielPluginInfo *info)
{
    g_return_val_if_fail(ARIEL_IS_PLUGIN_INFO(info), NULL);
    return info->author;
}

const char *
ariel_plugin_info_get_uri(ArielPluginInfo *info)
{
    g_return_val_if_fail(ARIEL_IS_PLUGIN_INFO(info), NULL);
    return info->uri;
}

ArielPluginManager *
ariel_plugin_manager_new(void)
{
    ArielPluginManager *manager = g_malloc0(sizeof(ArielPluginManager));
    
    // Initialize lilv world
    manager->world = lilv_world_new();
    lilv_world_load_all(manager->world);
    manager->plugins = lilv_world_get_all_plugins(manager->world);
    
    // Create list stores for UI
    manager->plugin_store = g_list_store_new(ARIEL_TYPE_PLUGIN_INFO);
    manager->active_plugin_store = g_list_store_new(ARIEL_TYPE_PLUGIN_INFO);
    
    ariel_plugin_manager_refresh(manager);
    
    return manager;
}

void
ariel_plugin_manager_refresh(ArielPluginManager *manager)
{
    if (!manager || !manager->world) return;
    
    // Clear existing store
    g_list_store_remove_all(manager->plugin_store);
    
    // Iterate through all LV2 plugins
    LILV_FOREACH(plugins, iter, manager->plugins) {
        const LilvPlugin *plugin = lilv_plugins_get(manager->plugins, iter);
        
        // Create plugin info object and add to store
        ArielPluginInfo *info = ariel_plugin_info_new(plugin);
        g_list_store_append(manager->plugin_store, info);
        
        g_print("Found LV2 plugin: %s by %s\n", 
                ariel_plugin_info_get_name(info),
                ariel_plugin_info_get_author(info));
        
        g_object_unref(info); // List store takes its own reference
    }
    
    g_print("Plugin manager refreshed with %u plugins\n", 
            g_list_model_get_n_items(G_LIST_MODEL(manager->plugin_store)));
}

void
ariel_plugin_manager_free(ArielPluginManager *manager)
{
    if (!manager) return;
    
    if (manager->plugin_store) {
        g_object_unref(manager->plugin_store);
    }
    if (manager->active_plugin_store) {
        g_object_unref(manager->active_plugin_store);
    }
    if (manager->world) {
        lilv_world_free(manager->world);
    }
    
    g_free(manager);
}