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
    if (!plugin) return NULL;
    
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

const LilvPlugin *
ariel_plugin_info_get_plugin(ArielPluginInfo *info)
{
    g_return_val_if_fail(ARIEL_IS_PLUGIN_INFO(info), NULL);
    return info->plugin;
}

ArielPluginManager *
ariel_plugin_manager_new(void)
{
    ArielPluginManager *manager = g_malloc0(sizeof(ArielPluginManager));
    
    // Initialize configuration
    manager->config = ariel_config_new();
    if (!manager->config) {
        g_warning("Failed to initialize configuration");
        g_free(manager);
        return NULL;
    }
    
    // Initialize lilv world
    manager->world = lilv_world_new();
    lilv_world_load_all(manager->world);
    manager->plugins = lilv_world_get_all_plugins(manager->world);
    
    // Create list stores for UI
    manager->plugin_store = g_list_store_new(ARIEL_TYPE_PLUGIN_INFO);
    manager->active_plugin_store = g_list_store_new(ARIEL_TYPE_ACTIVE_PLUGIN);
    
    // Try to load from cache first, otherwise refresh
    if (!ariel_plugin_manager_load_cache(manager)) {
        g_print("No valid cache found, scanning plugins...\n");
        ariel_plugin_manager_refresh(manager);
        ariel_plugin_manager_save_cache(manager);
    } else {
        g_print("Loaded plugins from cache\n");
    }
    
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

gboolean
ariel_plugin_manager_load_cache(ArielPluginManager *manager)
{
    if (!manager || !manager->config) return FALSE;
    
    const char *cache_file = ariel_config_get_cache_file(manager->config);
    if (!cache_file || !g_file_test(cache_file, G_FILE_TEST_EXISTS)) {
        return FALSE;
    }
    
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    
    if (!g_key_file_load_from_file(keyfile, cache_file, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to load cache file %s: %s", cache_file, error->message);
        g_error_free(error);
        g_key_file_free(keyfile);
        return FALSE;
    }
    
    // Check cache version and timestamp
    gchar *version = g_key_file_get_string(keyfile, "metadata", "version", NULL);
    if (!version || strcmp(version, "1.0") != 0) {
        g_warning("Incompatible cache version");
        g_free(version);
        g_key_file_free(keyfile);
        return FALSE;
    }
    g_free(version);
    
    // Get list of plugin groups (each plugin is a separate group)
    gsize n_groups;
    gchar **groups = g_key_file_get_groups(keyfile, &n_groups);
    
    // Clear existing store
    g_list_store_remove_all(manager->plugin_store);
    
    guint loaded_count = 0;
    
    for (gsize i = 0; i < n_groups; i++) {
        // Skip metadata group
        if (strcmp(groups[i], "metadata") == 0) continue;
        
        gchar *uri = g_key_file_get_string(keyfile, groups[i], "uri", NULL);
        if (!uri) continue;
        
        // Find the actual plugin in lilv world
        LilvNode *uri_node = lilv_new_uri(manager->world, uri);
        const LilvPlugin *plugin = lilv_plugins_get_by_uri(manager->plugins, uri_node);
        lilv_node_free(uri_node);
        
        if (plugin) {
            ArielPluginInfo *info = ariel_plugin_info_new(plugin);
            g_list_store_append(manager->plugin_store, info);
            g_object_unref(info);
            loaded_count++;
        }
        
        g_free(uri);
    }
    
    g_strfreev(groups);
    g_key_file_free(keyfile);
    
    g_print("Loaded %u plugins from cache\n", loaded_count);
    return loaded_count > 0;
}

void
ariel_plugin_manager_save_cache(ArielPluginManager *manager)
{
    if (!manager || !manager->config) return;
    
    const char *cache_file = ariel_config_get_cache_file(manager->config);
    if (!cache_file) return;
    
    GKeyFile *keyfile = g_key_file_new();
    
    // Add metadata
    g_key_file_set_string(keyfile, "metadata", "version", "1.0");
    g_key_file_set_int64(keyfile, "metadata", "timestamp", g_get_real_time() / G_USEC_PER_SEC);
    
    // Add each plugin as a separate group
    guint n_plugins = g_list_model_get_n_items(G_LIST_MODEL(manager->plugin_store));
    for (guint i = 0; i < n_plugins; i++) {
        ArielPluginInfo *info = g_list_model_get_item(G_LIST_MODEL(manager->plugin_store), i);
        if (!info) continue;
        
        // Use plugin index as group name
        gchar *group_name = g_strdup_printf("plugin_%u", i);
        
        g_key_file_set_string(keyfile, group_name, "uri", ariel_plugin_info_get_uri(info));
        g_key_file_set_string(keyfile, group_name, "name", ariel_plugin_info_get_name(info));
        g_key_file_set_string(keyfile, group_name, "author", ariel_plugin_info_get_author(info));
        
        g_free(group_name);
        g_object_unref(info);
    }
    
    // Save to file
    GError *error = NULL;
    gchar *data = g_key_file_to_data(keyfile, NULL, &error);
    
    if (data) {
        if (!g_file_set_contents(cache_file, data, -1, &error)) {
            g_warning("Failed to save cache file %s: %s", cache_file, error->message);
            g_error_free(error);
        } else {
            g_print("Saved plugin cache to %s\n", cache_file);
        }
        g_free(data);
    } else {
        g_warning("Failed to generate cache data: %s", error->message);
        g_error_free(error);
    }
    
    g_key_file_free(keyfile);
}

ArielActivePlugin *
ariel_plugin_manager_load_plugin(ArielPluginManager *manager, ArielPluginInfo *plugin_info, ArielAudioEngine *engine)
{
    if (!manager || !plugin_info || !engine) {
        g_warning("Invalid parameters for plugin loading");
        return NULL;
    }
    
    // Create active plugin instance
    ArielActivePlugin *active_plugin = ariel_active_plugin_new(plugin_info, engine);
    if (!active_plugin) {
        g_warning("Failed to create active plugin for %s", ariel_plugin_info_get_name(plugin_info));
        return NULL;
    }
    
    // Add to active plugins list
    g_list_store_append(manager->active_plugin_store, active_plugin);
    
    // Activate the plugin
    ariel_active_plugin_activate(active_plugin);
    
    g_print("Loaded and activated plugin: %s\n", ariel_active_plugin_get_name(active_plugin));
    
    return active_plugin;
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
    if (manager->config) {
        ariel_config_free(manager->config);
    }
    
    g_free(manager);
}

// Plugin Chain Preset Management Functions
gboolean
ariel_save_plugin_chain_preset(ArielPluginManager *manager, const char *preset_name, const char *preset_dir)
{
    if (!manager || !preset_name || !preset_dir || !manager->active_plugin_store) {
        return FALSE;
    }
    
    // Create preset directory if it doesn't exist
    g_mkdir_with_parents(preset_dir, 0755);
    
    // Create chain preset file path
    char *preset_filename = g_strdup_printf("%s.chain", preset_name);
    char *preset_path = g_build_filename(preset_dir, preset_filename, NULL);
    g_free(preset_filename);
    
    // Create key file for chain preset data
    GKeyFile *preset_file = g_key_file_new();
    
    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(manager->active_plugin_store));
    
    // Save chain metadata
    g_key_file_set_string(preset_file, "chain", "name", preset_name);
    g_key_file_set_integer(preset_file, "chain", "plugin_count", n_active);
    
    // Save each plugin in the chain
    for (guint i = 0; i < n_active; i++) {
        ArielActivePlugin *plugin = g_list_model_get_item(G_LIST_MODEL(manager->active_plugin_store), i);
        if (!plugin) continue;
        
        char *plugin_section = g_strdup_printf("plugin_%u", i);
        
        // Save plugin info
        const LilvPlugin *lilv_plugin = ariel_active_plugin_get_lilv_plugin(plugin);
        LilvNode *uri_node = lilv_plugin_get_uri(lilv_plugin);
        const char *plugin_uri = lilv_node_as_uri(uri_node);
        
        g_key_file_set_string(preset_file, plugin_section, "uri", plugin_uri);
        g_key_file_set_string(preset_file, plugin_section, "name", ariel_active_plugin_get_name(plugin));
        g_key_file_set_boolean(preset_file, plugin_section, "bypass", ariel_active_plugin_get_bypass(plugin));
        
        // Save parameters
        guint n_params = ariel_active_plugin_get_num_parameters(plugin);
        g_key_file_set_integer(preset_file, plugin_section, "param_count", n_params);
        
        for (guint j = 0; j < n_params; j++) {
            char *param_key = g_strdup_printf("param_%u", j);
            float value = ariel_active_plugin_get_parameter(plugin, j);
            g_key_file_set_double(preset_file, plugin_section, param_key, value);
            g_free(param_key);
        }
        
        g_free(plugin_section);
        g_object_unref(plugin);
    }
    
    // Save preset file
    gsize length;
    char *preset_data = g_key_file_to_data(preset_file, &length, NULL);
    gboolean success = g_file_set_contents(preset_path, preset_data, length, NULL);
    
    // Cleanup
    g_free(preset_data);
    g_free(preset_path);
    g_key_file_free(preset_file);
    
    if (success) {
        g_print("Saved plugin chain preset '%s' with %u plugins\n", preset_name, n_active);
    }
    
    return success;
}

gboolean
ariel_load_plugin_chain_preset(ArielPluginManager *manager, ArielAudioEngine *engine, const char *preset_path)
{
    if (!manager || !engine || !preset_path || !g_file_test(preset_path, G_FILE_TEST_EXISTS)) {
        return FALSE;
    }
    
    GKeyFile *preset_file = g_key_file_new();
    GError *error = NULL;
    
    // Load preset file
    if (!g_key_file_load_from_file(preset_file, preset_path, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to load chain preset file %s: %s", preset_path, error->message);
        g_error_free(error);
        g_key_file_free(preset_file);
        return FALSE;
    }
    
    // Clear current active plugins
    g_list_store_remove_all(manager->active_plugin_store);
    
    // Load chain metadata
    gint plugin_count = g_key_file_get_integer(preset_file, "chain", "plugin_count", NULL);
    
    // Load each plugin in the chain
    for (gint i = 0; i < plugin_count; i++) {
        char *plugin_section = g_strdup_printf("plugin_%d", i);
        
        // Get plugin URI
        char *plugin_uri = g_key_file_get_string(preset_file, plugin_section, "uri", NULL);
        if (!plugin_uri) {
            g_free(plugin_section);
            continue;
        }
        
        // Find plugin info by URI
        ArielPluginInfo *plugin_info = NULL;
        guint n_plugins = g_list_model_get_n_items(G_LIST_MODEL(manager->plugin_store));
        
        for (guint j = 0; j < n_plugins; j++) {
            ArielPluginInfo *info = g_list_model_get_item(G_LIST_MODEL(manager->plugin_store), j);
            if (info && strcmp(ariel_plugin_info_get_uri(info), plugin_uri) == 0) {
                plugin_info = info;
                break;
            }
            if (info) g_object_unref(info);
        }
        
        if (!plugin_info) {
            g_warning("Plugin not found for URI: %s", plugin_uri);
            g_free(plugin_uri);
            g_free(plugin_section);
            continue;
        }
        
        // Load the plugin
        ArielActivePlugin *active_plugin = ariel_plugin_manager_load_plugin(manager, plugin_info, engine);
        if (!active_plugin) {
            g_warning("Failed to load plugin: %s", plugin_uri);
            g_object_unref(plugin_info);
            g_free(plugin_uri);
            g_free(plugin_section);
            continue;
        }
        
        // Load bypass state
        if (g_key_file_has_key(preset_file, plugin_section, "bypass", NULL)) {
            gboolean bypass = g_key_file_get_boolean(preset_file, plugin_section, "bypass", NULL);
            ariel_active_plugin_set_bypass(active_plugin, bypass);
        }
        
        // Load parameters
        gint param_count = g_key_file_get_integer(preset_file, plugin_section, "param_count", NULL);
        guint num_parameters = ariel_active_plugin_get_num_parameters(active_plugin);
        
        for (gint j = 0; j < param_count && j < (gint)num_parameters; j++) {
            char *param_key = g_strdup_printf("param_%d", j);
            
            if (g_key_file_has_key(preset_file, plugin_section, param_key, NULL)) {
                gdouble value = g_key_file_get_double(preset_file, plugin_section, param_key, NULL);
                ariel_active_plugin_set_parameter(active_plugin, j, (float)value);
            }
            
            g_free(param_key);
        }
        
        g_object_unref(plugin_info);
        g_object_unref(active_plugin);
        g_free(plugin_uri);
        g_free(plugin_section);
    }
    
    g_key_file_free(preset_file);
    
    char *preset_name = g_path_get_basename(preset_path);
    if (g_str_has_suffix(preset_name, ".chain")) {
        preset_name[strlen(preset_name) - 6] = '\0'; // Remove .chain extension
    }
    g_print("Loaded plugin chain preset '%s' with %d plugins\n", preset_name, plugin_count);
    g_free(preset_name);
    
    return TRUE;
}

char **
ariel_list_plugin_chain_presets(const char *preset_dir)
{
    if (!preset_dir || !g_file_test(preset_dir, G_FILE_TEST_IS_DIR)) {
        return NULL;
    }
    
    GDir *dir = g_dir_open(preset_dir, 0, NULL);
    if (!dir) {
        return NULL;
    }
    
    GPtrArray *preset_list = g_ptr_array_new();
    const char *filename;
    
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (g_str_has_suffix(filename, ".chain")) {
            // Remove .chain extension from filename
            char *preset_name = g_strndup(filename, strlen(filename) - 6);
            g_ptr_array_add(preset_list, preset_name);
        }
    }
    
    g_dir_close(dir);
    
    // Convert to NULL-terminated string array
    g_ptr_array_add(preset_list, NULL);
    return (char **)g_ptr_array_free(preset_list, FALSE);
}

void
ariel_free_plugin_chain_preset_list(char **preset_list)
{
    if (!preset_list) return;
    
    for (int i = 0; preset_list[i] != NULL; i++) {
        g_free(preset_list[i]);
    }
    g_free(preset_list);
}