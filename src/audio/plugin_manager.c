#include "ariel.h"

ArielPluginManager *
ariel_plugin_manager_new(void)
{
    ArielPluginManager *manager = g_malloc0(sizeof(ArielPluginManager));
    
    // Initialize lilv world
    manager->world = lilv_world_new();
    lilv_world_load_all(manager->world);
    manager->plugins = lilv_world_get_all_plugins(manager->world);
    
    // Create list stores for UI
    manager->plugin_store = g_list_store_new(G_TYPE_OBJECT); // TODO: Define plugin object type
    manager->active_plugin_store = g_list_store_new(G_TYPE_OBJECT);
    
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
        
        // Get plugin name
        LilvNode *name = lilv_plugin_get_name(plugin);
        if (name) {
            const char *plugin_name = lilv_node_as_string(name);
            g_print("Found LV2 plugin: %s\n", plugin_name);
            
            // TODO: Create plugin object and add to store
            // For now, just print the name
            
            lilv_node_free(name);
        }
    }
    
    g_print("Plugin manager refreshed\n");
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