#ifndef ARIEL_H
#define ARIEL_H

#include <gtk/gtk.h>
#include <lilv/lilv.h>
#include <jack/jack.h>

#define ARIEL_APP_ID "com.github.djshaji.ariel"

// Forward declarations
typedef struct _ArielApp ArielApp;
typedef struct _ArielWindow ArielWindow;
typedef struct _ArielAudioEngine ArielAudioEngine;
typedef struct _ArielPluginManager ArielPluginManager;
typedef struct _ArielPluginInfo ArielPluginInfo;

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
};

#define ARIEL_TYPE_PLUGIN_INFO (ariel_plugin_info_get_type())
G_DECLARE_FINAL_TYPE(ArielPluginInfo, ariel_plugin_info, ARIEL, PLUGIN_INFO, GObject)

// Plugin manager structure
struct _ArielPluginManager {
    LilvWorld *world;
    const LilvPlugins *plugins;
    GListStore *plugin_store;
    GListStore *active_plugin_store;
};

// Function prototypes

// Application
ArielApp *ariel_app_new(void);
ArielAudioEngine *ariel_app_get_audio_engine(ArielApp *app);
ArielPluginManager *ariel_app_get_plugin_manager(ArielApp *app);

// Window
ArielWindow *ariel_window_new(ArielApp *app);
void ariel_window_setup_ui(ArielWindow *window);

// UI Components
GtkWidget *ariel_create_header_bar(ArielWindow *window);
GtkWidget *ariel_create_plugin_list(ArielWindow *window);
GtkWidget *ariel_create_mixer(ArielWindow *window);
GtkWidget *ariel_create_mixer_channel(const char *name, int channel_id);
GtkWidget *ariel_create_transport(ArielWindow *window);

// Plugin list callbacks
void setup_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
void bind_plugin_list_item(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data);

// Audio Engine
ArielAudioEngine *ariel_audio_engine_new(void);
gboolean ariel_audio_engine_start(ArielAudioEngine *engine);
void ariel_audio_engine_stop(ArielAudioEngine *engine);
void ariel_audio_engine_free(ArielAudioEngine *engine);

// Plugin Info
ArielPluginInfo *ariel_plugin_info_new(const LilvPlugin *plugin);
const char *ariel_plugin_info_get_name(ArielPluginInfo *info);
const char *ariel_plugin_info_get_author(ArielPluginInfo *info);
const char *ariel_plugin_info_get_uri(ArielPluginInfo *info);

// Plugin Manager
ArielPluginManager *ariel_plugin_manager_new(void);
void ariel_plugin_manager_refresh(ArielPluginManager *manager);
void ariel_plugin_manager_free(ArielPluginManager *manager);

// JACK callbacks
int ariel_jack_process_callback(jack_nframes_t nframes, void *arg);
void ariel_jack_shutdown_callback(void *arg);

#endif // ARIEL_H