#include "ariel.h"

// Forward declaration
void ariel_free_lv2_features(LV2_Feature **features);

// Worker schedule work item structure
typedef struct {
    ArielActivePlugin *plugin;
    uint32_t size;
    void *data;
} ArielWorkerWork;

// Worker thread function
static void
ariel_worker_thread_func(gpointer data, gpointer user_data)
{
    ArielWorkerWork *work = (ArielWorkerWork *)data;
    ArielWorkerSchedule *worker = (ArielWorkerSchedule *)user_data;
    
    if (!work || !work->plugin) {
        g_free(work);
        return;
    }
    
    // In a full implementation, this would call the plugin's work interface
    // For now, we'll just simulate work processing
    g_print("Processing worker task for plugin, size: %u\n", work->size);
    
    // Simulate some work
    g_usleep(1000); // 1ms of work
    
    // Send response back to plugin (placeholder)
    ariel_worker_respond(work->plugin, work->size, work->data);
    
    // Cleanup
    g_free(work->data);
    g_free(work);
}

// Create new worker schedule
ArielWorkerSchedule *
ariel_worker_schedule_new(void)
{
    ArielWorkerSchedule *worker = g_malloc0(sizeof(ArielWorkerSchedule));
    
    // Create thread pool with 2 worker threads
    worker->thread_pool = g_thread_pool_new(ariel_worker_thread_func,
                                           worker,
                                           2, // max threads
                                           FALSE, // exclusive
                                           NULL);
    
    g_mutex_init(&worker->work_mutex);
    worker->work_queue = g_queue_new();
    
    g_print("Created LV2 worker schedule with thread pool\n");
    return worker;
}

// Free worker schedule
void
ariel_worker_schedule_free(ArielWorkerSchedule *worker)
{
    if (!worker) return;
    
    // Shutdown thread pool
    if (worker->thread_pool) {
        g_thread_pool_free(worker->thread_pool, FALSE, TRUE);
    }
    
    // Clear work queue
    if (worker->work_queue) {
        ArielWorkerWork *work;
        while ((work = g_queue_pop_head(worker->work_queue)) != NULL) {
            g_free(work->data);
            g_free(work);
        }
        g_queue_free(worker->work_queue);
    }
    
    g_mutex_clear(&worker->work_mutex);
    g_free(worker);
}

// LV2 Worker Schedule interface implementation
LV2_Worker_Status
ariel_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data)
{
    ArielWorkerSchedule *worker = (ArielWorkerSchedule *)handle;
    if (!worker || !data) return LV2_WORKER_ERR_UNKNOWN;
    
    // Create work item
    ArielWorkerWork *work = g_malloc0(sizeof(ArielWorkerWork));
    work->plugin = worker->plugin;
    work->size = size;
    work->data = g_malloc(size);
    memcpy(work->data, data, size);
    
    // Schedule work in thread pool
    GError *error = NULL;
    gboolean success = g_thread_pool_push(worker->thread_pool, work, &error);
    
    if (!success) {
        g_warning("Failed to schedule worker task: %s", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        g_free(work->data);
        g_free(work);
        return LV2_WORKER_ERR_UNKNOWN;
    }
    
    g_print("Scheduled worker task, size: %u\n", size);
    return LV2_WORKER_SUCCESS;
}

// Worker response function (called from worker thread)
void
ariel_worker_respond(ArielActivePlugin *plugin, uint32_t size, const void *data)
{
    if (!plugin) return;
    
    // In a full implementation, this would call the plugin's work_response interface
    // through a safe mechanism (like a lock-free queue to the audio thread)
    g_print("Worker response for plugin, size: %u\n", size);
}

// LV2 State interface implementation
static LV2_State_Status
ariel_state_store(LV2_State_Handle handle,
                 uint32_t key,
                 const void* value,
                 size_t size,
                 uint32_t type,
                 uint32_t flags)
{
    ArielPluginManager *manager = (ArielPluginManager *)handle;
    if (!manager || !value) return LV2_STATE_ERR_UNKNOWN;
    
    // For now, just log state store requests
    // In a full implementation, we'd store this in the plugin's state
    g_print("Plugin state store: key=%u, size=%zu, type=%u, flags=%u\n", 
            key, size, type, flags);
    return LV2_STATE_SUCCESS;
}

static const void*
ariel_state_retrieve(LV2_State_Handle handle,
                    uint32_t key,
                    size_t* size,
                    uint32_t* type,
                    uint32_t* flags)
{
    ArielPluginManager *manager = (ArielPluginManager *)handle;
    if (!manager) return NULL;
    
    // For now, return NULL (no stored state)
    // In a full implementation, we'd retrieve from plugin's stored state
    g_print("Plugin state retrieve: key=%u\n", key);
    if (size) *size = 0;
    if (type) *type = 0;
    if (flags) *flags = 0;
    return NULL;
}

static char*
ariel_state_make_path(LV2_State_Handle handle, const char* path)
{
    ArielPluginManager *manager = (ArielPluginManager *)handle;
    if (!manager || !path) return NULL;
    
    // Create state directory if it doesn't exist
    ArielConfig *config = manager->config;
    const char *config_dir = ariel_config_get_dir(config);
    char *state_dir = g_build_filename(config_dir, "plugin_state", NULL);
    g_mkdir_with_parents(state_dir, 0755);
    
    char *full_path = g_build_filename(state_dir, path, NULL);
    g_free(state_dir);
    
    g_print("Plugin state make_path: %s -> %s\n", path, full_path);
    return full_path;
}

// URID Map Implementation
ArielURIDMap *
ariel_urid_map_new(void)
{
    ArielURIDMap *map = g_malloc0(sizeof(ArielURIDMap));
    map->uri_to_id = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    map->id_to_uri = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    map->next_id = 1; // Start from 1, as 0 is reserved for "no value"
    return map;
}

void
ariel_urid_map_free(ArielURIDMap *map)
{
    if (!map) return;
    
    g_hash_table_destroy(map->uri_to_id);
    g_hash_table_destroy(map->id_to_uri);
    g_free(map);
}

LV2_URID
ariel_urid_map(LV2_URID_Map_Handle handle, const char *uri)
{
    ArielURIDMap *map = (ArielURIDMap *)handle;
    if (!map || !uri) return 0;
    
    // Check if URI already exists
    gpointer existing_id = g_hash_table_lookup(map->uri_to_id, uri);
    if (existing_id) {
        return GPOINTER_TO_UINT(existing_id);
    }
    
    // Create new mapping
    LV2_URID new_id = map->next_id++;
    char *uri_copy = g_strdup(uri);
    char *uri_copy2 = g_strdup(uri);
    
    g_hash_table_insert(map->uri_to_id, uri_copy, GUINT_TO_POINTER(new_id));
    g_hash_table_insert(map->id_to_uri, GUINT_TO_POINTER(new_id), uri_copy2);
    
    g_print("URID Map: %s -> %u\n", uri, new_id);
    return new_id;
}

const char *
ariel_urid_unmap(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
    ArielURIDMap *map = (ArielURIDMap *)handle;
    if (!map || urid == 0) return NULL;
    
    return (const char *)g_hash_table_lookup(map->id_to_uri, GUINT_TO_POINTER(urid));
}

// LV2 Atom Path support functions
LV2_URID
ariel_get_atom_path_urid(ArielPluginManager *manager)
{
    if (!manager || !manager->urid_map) return 0;
    return ariel_urid_map(manager->urid_map, LV2_ATOM__Path);
}

// Map absolute path to plugin-accessible path
char *
ariel_map_absolute_path(LV2_State_Handle handle, const char *absolute_path)
{
    ArielPluginManager *manager = (ArielPluginManager *)handle;
    if (!manager || !absolute_path) return NULL;
    
    // Create plugin-accessible path within state directory
    ArielConfig *config = manager->config;
    const char *config_dir = ariel_config_get_dir(config);
    char *state_dir = g_build_filename(config_dir, "plugin_state", NULL);
    
    // Generate safe filename from absolute path
    char *basename = g_path_get_basename(absolute_path);
    char *safe_path = g_build_filename(state_dir, basename, NULL);
    
    // Copy file to state directory if it doesn't exist
    if (!g_file_test(safe_path, G_FILE_TEST_EXISTS) && g_file_test(absolute_path, G_FILE_TEST_EXISTS)) {
        GError *error = NULL;
        GFile *source = g_file_new_for_path(absolute_path);
        GFile *dest = g_file_new_for_path(safe_path);
        
        if (!g_file_copy(source, dest, G_FILE_COPY_NONE, NULL, NULL, NULL, &error)) {
            g_warning("Failed to copy file %s to %s: %s", absolute_path, safe_path, error->message);
            g_error_free(error);
            g_free(safe_path);
            safe_path = NULL;
        } else {
            g_print("Mapped absolute path: %s -> %s\n", absolute_path, safe_path);
        }
        
        g_object_unref(source);
        g_object_unref(dest);
    }
    
    g_free(basename);
    g_free(state_dir);
    return safe_path;
}

// Map abstract path to absolute path
char *
ariel_map_abstract_path(LV2_State_Handle handle, const char *abstract_path)
{
    ArielPluginManager *manager = (ArielPluginManager *)handle;
    if (!manager || !abstract_path) return NULL;
    
    // Convert abstract path to absolute path within state directory
    ArielConfig *config = manager->config;
    const char *config_dir = ariel_config_get_dir(config);
    char *state_dir = g_build_filename(config_dir, "plugin_state", NULL);
    char *absolute_path = g_build_filename(state_dir, abstract_path, NULL);
    
    g_print("Mapped abstract path: %s -> %s\n", abstract_path, absolute_path);
    
    g_free(state_dir);
    return absolute_path;
}

// Create LV2 feature array
LV2_Feature **
ariel_create_lv2_features(ArielPluginManager *manager, ArielAudioEngine *engine)
{
    if (!manager || !engine) return NULL;
    
    // Allocate features array (map, unmap, options, makePath, mapPath, schedule, NULL terminator)
    LV2_Feature **features = g_malloc0(7 * sizeof(LV2_Feature *));
    
    // URID Map feature
    LV2_URID_Map *map_feature = g_malloc0(sizeof(LV2_URID_Map));
    map_feature->handle = manager->urid_map;
    map_feature->map = ariel_urid_map;
    
    features[0] = g_malloc0(sizeof(LV2_Feature));
    features[0]->URI = LV2_URID__map;
    features[0]->data = map_feature;
    
    // URID Unmap feature
    LV2_URID_Unmap *unmap_feature = g_malloc0(sizeof(LV2_URID_Unmap));
    unmap_feature->handle = manager->urid_map;
    unmap_feature->unmap = ariel_urid_unmap;
    
    features[1] = g_malloc0(sizeof(LV2_Feature));
    features[1]->URI = LV2_URID__unmap;
    features[1]->data = unmap_feature;
    
    // Basic Options feature - create empty options array to satisfy plugins
    LV2_Options_Option *options = g_malloc0(1 * sizeof(LV2_Options_Option));
    // Just create terminator - many plugins just check for presence of feature
    options[0].key = 0;
    options[0].value = NULL;
    
    features[2] = g_malloc0(sizeof(LV2_Feature));
    features[2]->URI = LV2_OPTIONS__options;
    features[2]->data = options;
    
    // State Make Path feature
    LV2_State_Make_Path *make_path = g_malloc0(sizeof(LV2_State_Make_Path));
    make_path->handle = manager;
    make_path->path = ariel_state_make_path;
    
    features[3] = g_malloc0(sizeof(LV2_Feature));
    features[3]->URI = LV2_STATE__makePath;
    features[3]->data = make_path;
    
    // State Map Path feature
    LV2_State_Map_Path *map_path = g_malloc0(sizeof(LV2_State_Map_Path));
    map_path->handle = manager;
    map_path->absolute_path = ariel_map_absolute_path;
    map_path->abstract_path = ariel_map_abstract_path;
    
    features[4] = g_malloc0(sizeof(LV2_Feature));
    features[4]->URI = LV2_STATE__mapPath;
    features[4]->data = map_path;
    
    // Worker Schedule feature
    LV2_Worker_Schedule *schedule = g_malloc0(sizeof(LV2_Worker_Schedule));
    schedule->handle = manager->worker_schedule;
    schedule->schedule_work = ariel_worker_schedule;
    
    features[5] = g_malloc0(sizeof(LV2_Feature));
    features[5]->URI = LV2_WORKER__schedule;
    features[5]->data = schedule;
    
    // Pre-map important Atom URIs including Path
    ariel_urid_map(manager->urid_map, LV2_ATOM__Path);
    ariel_urid_map(manager->urid_map, LV2_ATOM__String);
    ariel_urid_map(manager->urid_map, LV2_ATOM__URI);
    
    // NULL terminator
    features[6] = NULL;
    
    g_print("Created LV2 features: URID Map/Unmap, Options, State Make Path, Map Path, Worker Schedule\n");
    return features;
}

void
ariel_free_lv2_features(LV2_Feature **features)
{
    if (!features) return;
    
    for (int i = 0; features[i] != NULL; i++) {
        if (g_strcmp0(features[i]->URI, LV2_OPTIONS__options) == 0) {
            // Free options array
            g_free(features[i]->data);
        } else if (g_strcmp0(features[i]->URI, LV2_WORKER__schedule) == 0) {
            // Free worker schedule
            g_free(features[i]->data);
        } else {
            // Free other feature data
            g_free(features[i]->data);
        }
        g_free(features[i]);
    }
    g_free(features);
}

// ArielPluginInfo GObject implementation
struct _ArielPluginInfo {
    GObject parent;
    char *name;
    char *author;
    char *uri;
    char *category;
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
    g_free(info->category);
    
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
    info->category = NULL;
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
    
    // Get plugin category from LV2 class
    const LilvPluginClass *plugin_class = lilv_plugin_get_class(plugin);
    if (plugin_class) {
        const LilvNode *class_label = lilv_plugin_class_get_label(plugin_class);
        if (class_label) {
            info->category = g_strdup(lilv_node_as_string(class_label));
        } else {
            info->category = g_strdup("Unknown");
        }
    } else {
        info->category = g_strdup("Unknown");
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

const char *
ariel_plugin_info_get_category(ArielPluginInfo *info)
{
    g_return_val_if_fail(ARIEL_IS_PLUGIN_INFO(info), NULL);
    return info->category;
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
    
    // Initialize URID map
    manager->urid_map = ariel_urid_map_new();
    if (!manager->urid_map) {
        g_warning("Failed to initialize URID map");
        ariel_config_free(manager->config);
        g_free(manager);
        return NULL;
    }
    
    // Initialize worker schedule
    manager->worker_schedule = ariel_worker_schedule_new();
    if (!manager->worker_schedule) {
        g_warning("Failed to initialize worker schedule");
        ariel_urid_map_free(manager->urid_map);
        ariel_config_free(manager->config);
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
    
    // Initialize features as NULL - will be created when needed with engine reference
    manager->features = NULL;
    
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
        g_key_file_set_string(keyfile, group_name, "category", ariel_plugin_info_get_category(info));
        
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
    
    // Set plugin reference in worker schedule for this plugin
    if (manager->worker_schedule) {
        manager->worker_schedule->plugin = active_plugin;
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
    
    if (manager->plugin_store && G_IS_OBJECT(manager->plugin_store)) {
        g_object_unref(manager->plugin_store);
        manager->plugin_store = NULL;
    }
    if (manager->active_plugin_store && G_IS_OBJECT(manager->active_plugin_store)) {
        g_object_unref(manager->active_plugin_store);
        manager->active_plugin_store = NULL;
    }
    if (manager->features) {
        ariel_free_lv2_features(manager->features);
    }
    if (manager->worker_schedule) {
        ariel_worker_schedule_free(manager->worker_schedule);
    }
    if (manager->urid_map) {
        ariel_urid_map_free(manager->urid_map);
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
        
        // Get plugin URI
        const LilvPlugin *lilv_plugin = ariel_active_plugin_get_lilv_plugin(plugin);
        const LilvNode *uri_node = lilv_plugin_get_uri(lilv_plugin);
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