#ifndef ARIEL_H
#define ARIEL_H

#include <gtk/gtk.h>
#include <lilv/lilv.h>
#include <jack/jack.h>

// LV2 Feature headers
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/options/options.h>
#include <lv2/state/state.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/patch/patch.h>
#include <lv2/worker/worker.h>
#include <lv2/log/log.h>

// Ariel logging system
#include "ariel_log.h"

#define ARIEL_APP_ID "com.github.djshaji.ariel"
#define APP "Ariel"

// Forward declarations
typedef struct _ArielApp ArielApp;
typedef struct _ArielWindow ArielWindow;
typedef struct _ArielAudioEngine ArielAudioEngine;
typedef struct _ArielPluginManager ArielPluginManager;
typedef struct _ArielPluginInfo ArielPluginInfo;
typedef struct _ArielConfig ArielConfig;
typedef struct _ArielActivePlugin ArielActivePlugin;

#define ARIEL_TYPE_APP (ariel_app_get_type())
G_DECLARE_FINAL_TYPE(ArielApp, ariel_app, ARIEL, APP, GtkApplication)

// Main window structure
struct _ArielWindow {
    GtkApplicationWindow parent;
    
    // UI elements
    GtkWidget *header_bar;
    GtkWidget *audio_toggle;
    GtkWidget *main_paned;
    GtkWidget *plugin_list;
    GtkWidget *active_plugins;
    GtkWidget *mixer_box;
    GtkWidget *transport_box;
    
    // Transport controls
    GtkWidget *play_button;
    GtkWidget *stop_button;
    GtkWidget *record_button;
    gboolean is_playing;
    gboolean is_recording;
    
    // References
    ArielApp *app;
};

// Audio engine structure
struct _ArielAudioEngine {
    jack_client_t *client;
    jack_port_t *input_ports[2];
    jack_port_t *output_ports[2];
    gboolean active;
    gfloat sample_rate;
    gint buffer_size;
    ArielPluginManager *plugin_manager;  // Reference to plugin manager for processing
};

#define ARIEL_TYPE_PLUGIN_INFO (ariel_plugin_info_get_type())
G_DECLARE_FINAL_TYPE(ArielPluginInfo, ariel_plugin_info, ARIEL, PLUGIN_INFO, GObject)

#define ARIEL_TYPE_ACTIVE_PLUGIN (ariel_active_plugin_get_type())
G_DECLARE_FINAL_TYPE(ArielActivePlugin, ariel_active_plugin, ARIEL, ACTIVE_PLUGIN, GObject)

// Configuration structure
struct _ArielConfig {
    char *config_dir;
    char *cache_file;
};

// URID map structure for LV2 features
typedef struct {
    GHashTable *uri_to_id;
    GHashTable *id_to_uri;
    uint32_t next_id;
} ArielURIDMap;

// Worker response item structure
typedef struct {
    ArielActivePlugin *plugin;
    uint32_t size;
    void *data;
} ArielWorkerResponse;

// Worker schedule structure for LV2 worker feature
typedef struct {
    GThreadPool *thread_pool;
    GMutex work_mutex;
    GQueue *work_queue;
    GMutex response_mutex;
    GQueue *response_queue;
    ArielActivePlugin *plugin;
} ArielWorkerSchedule;

// Plugin manager structure
struct _ArielPluginManager {
    LilvWorld *world;
    const LilvPlugins *plugins;
    GListStore *plugin_store;
    GListStore *active_plugin_store;
    ArielConfig *config;
    ArielURIDMap *urid_map;
    ArielWorkerSchedule *worker_schedule;
    LV2_Feature **features;
};

// Function prototypes

// Application
ArielApp *ariel_app_new(void);
ArielAudioEngine *ariel_app_get_audio_engine(ArielApp *app);
ArielPluginManager *ariel_app_get_plugin_manager(ArielApp *app);
void ariel_load_custom_css(void);

// Window
ArielWindow *ariel_window_new(ArielApp *app);
void ariel_window_setup_ui(ArielWindow *window);

// UI Components
GtkWidget *ariel_create_header_bar(ArielWindow *window);
GtkWidget *ariel_create_plugin_list(ArielWindow *window);
GtkWidget *ariel_create_mixer(ArielWindow *window);
GtkWidget *ariel_create_mixer_channel(const char *name, int channel_id);
GtkWidget *ariel_create_transport(ArielWindow *window);
void ariel_show_settings_dialog(ArielWindow *window);

// Theme Management
void ariel_save_theme_preference(const char *theme_name);
char *ariel_load_theme_preference(void);
void ariel_apply_saved_theme(void);
void ariel_transport_play(ArielWindow *window);
void ariel_transport_stop(ArielWindow *window);
void ariel_transport_record(ArielWindow *window);
void ariel_transport_update_ui(ArielWindow *window);

// Active Plugins View
GtkWidget *ariel_create_active_plugins_view(ArielWindow *window);
GtkWidget *ariel_create_active_plugin_widget(ArielActivePlugin *plugin, ArielWindow *window);
void ariel_update_active_plugins_view(ArielWindow *window);

// Plugin list callbacks
void setup_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
void bind_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data);

// Audio Engine
ArielAudioEngine *ariel_audio_engine_new(void);
gboolean ariel_audio_engine_start(ArielAudioEngine *engine);
void ariel_audio_engine_stop(ArielAudioEngine *engine);
void ariel_audio_engine_free(ArielAudioEngine *engine);
void ariel_audio_engine_set_plugin_manager(ArielAudioEngine *engine, ArielPluginManager *manager);

// Plugin Info
ArielPluginInfo *ariel_plugin_info_new(const LilvPlugin *plugin);
const char *ariel_plugin_info_get_name(ArielPluginInfo *info);
const char *ariel_plugin_info_get_author(ArielPluginInfo *info);
const char *ariel_plugin_info_get_uri(ArielPluginInfo *info);
const char *ariel_plugin_info_get_category(ArielPluginInfo *info);
const LilvPlugin *ariel_plugin_info_get_plugin(ArielPluginInfo *info);
const char *ariel_plugin_info_get_author(ArielPluginInfo *info);
const char *ariel_plugin_info_get_uri(ArielPluginInfo *info);

// Active Plugin
ArielActivePlugin *ariel_active_plugin_new(ArielPluginInfo *plugin_info, ArielAudioEngine *engine);
void ariel_active_plugin_process(ArielActivePlugin *plugin, jack_nframes_t nframes);
void ariel_active_plugin_activate(ArielActivePlugin *plugin);
void ariel_active_plugin_deactivate(ArielActivePlugin *plugin);  
const char *ariel_active_plugin_get_name(ArielActivePlugin *plugin);
gboolean ariel_active_plugin_is_active(ArielActivePlugin *plugin);
void ariel_active_plugin_connect_audio_ports(ArielActivePlugin *plugin, float **input_buffers, float **output_buffers);

// Parameter Control
const LilvPlugin *ariel_active_plugin_get_lilv_plugin(ArielActivePlugin *plugin);
uint32_t ariel_active_plugin_get_num_parameters(ArielActivePlugin *plugin);
float ariel_active_plugin_get_parameter(ArielActivePlugin *plugin, uint32_t index);
void ariel_active_plugin_set_parameter(ArielActivePlugin *plugin, uint32_t index, float value);
uint32_t ariel_active_plugin_get_control_port_index(ArielActivePlugin *plugin, uint32_t param_index);
gboolean ariel_active_plugin_is_mono(ArielActivePlugin *plugin);
guint ariel_active_plugin_get_n_audio_inputs(ArielActivePlugin *plugin);
guint ariel_active_plugin_get_n_audio_outputs(ArielActivePlugin *plugin);
void ariel_active_plugin_set_bypass(ArielActivePlugin *plugin, gboolean bypass);
gboolean ariel_active_plugin_get_bypass(ArielActivePlugin *plugin);

// Atom Messaging for File Parameters
void ariel_active_plugin_set_file_parameter(ArielActivePlugin *plugin, const char *file_path);
void ariel_active_plugin_set_file_parameter_with_uri(ArielActivePlugin *plugin, const char *file_path, const char *parameter_uri);
gboolean ariel_active_plugin_supports_file_parameters(ArielActivePlugin *plugin);

// Preset Management
gboolean ariel_active_plugin_save_preset(ArielActivePlugin *plugin, const char *preset_name, const char *preset_dir);
gboolean ariel_active_plugin_load_preset(ArielActivePlugin *plugin, const char *preset_path);
char **ariel_active_plugin_list_presets(ArielActivePlugin *plugin, const char *preset_dir);
void ariel_active_plugin_free_preset_list(char **preset_list);

// Plugin Chain Presets
gboolean ariel_save_plugin_chain_preset(ArielPluginManager *manager, const char *preset_name, const char *preset_dir);
gboolean ariel_load_plugin_chain_preset(ArielPluginManager *manager, ArielAudioEngine *engine, const char *preset_path);
char **ariel_list_plugin_chain_presets(const char *preset_dir);
void ariel_free_plugin_chain_preset_list(char **preset_list);

GtkWidget *ariel_create_parameter_controls(ArielActivePlugin *plugin);

// Configuration
ArielConfig *ariel_config_new(void);
void ariel_config_free(ArielConfig *config);
const char *ariel_config_get_dir(ArielConfig *config);
const char *ariel_config_get_cache_file(ArielConfig *config);

// Plugin Manager
ArielPluginManager *ariel_plugin_manager_new(void);
void ariel_plugin_manager_refresh(ArielPluginManager *manager);
gboolean ariel_plugin_manager_load_cache(ArielPluginManager *manager);
void ariel_plugin_manager_save_cache(ArielPluginManager *manager);
ArielActivePlugin *ariel_plugin_manager_load_plugin(ArielPluginManager *manager, ArielPluginInfo *plugin_info, ArielAudioEngine *engine);
void ariel_plugin_manager_free(ArielPluginManager *manager);

// URID Map support
ArielURIDMap *ariel_urid_map_new(void);
void ariel_urid_map_free(ArielURIDMap *map);
LV2_URID ariel_urid_map(LV2_URID_Map_Handle handle, const char *uri);
const char *ariel_urid_unmap(LV2_URID_Unmap_Handle handle, LV2_URID urid);
LV2_Feature **ariel_create_lv2_features(ArielPluginManager *manager, ArielAudioEngine *engine);
void ariel_free_lv2_features(LV2_Feature **features);

// LV2 Atom Path support
char *ariel_map_absolute_path(LV2_State_Handle handle, const char *absolute_path);
char *ariel_map_abstract_path(LV2_State_Handle handle, const char *abstract_path);
LV2_URID ariel_get_atom_path_urid(ArielPluginManager *manager);

// LV2 Worker Schedule support
ArielWorkerSchedule *ariel_worker_schedule_new(void);
void ariel_worker_schedule_free(ArielWorkerSchedule *worker);
LV2_Worker_Status ariel_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);
void ariel_worker_respond(ArielActivePlugin *plugin, uint32_t size, const void *data);
LV2_Worker_Status ariel_worker_respond_callback(LV2_Worker_Respond_Handle handle, uint32_t size, const void *data);
void ariel_worker_process_responses(ArielWorkerSchedule *worker);

// Active Plugin Worker Interface
void ariel_active_plugin_process_worker_responses(ArielActivePlugin *plugin);
void ariel_active_plugin_process_ui_messages(ArielActivePlugin *plugin);
gboolean ariel_active_plugin_has_work_interface(ArielActivePlugin *plugin);
LilvInstance *ariel_active_plugin_get_instance(ArielActivePlugin *plugin);

// JACK callbacks
int ariel_jack_process_callback(jack_nframes_t nframes, void *arg);
void ariel_jack_shutdown_callback(void *arg);

// WASAPI support (Windows only)
#ifdef _WIN32
gboolean ariel_wasapi_start(ArielAudioEngine *engine);
void ariel_wasapi_stop(ArielAudioEngine *engine);
GList *ariel_wasapi_enumerate_devices(gboolean input_devices);
#endif

#endif // ARIEL_H