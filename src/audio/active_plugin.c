#include "ariel.h"
#include <string.h>

// ArielActivePlugin structure
struct _ArielActivePlugin {
    GObject parent;
    
    // Plugin info
    ArielPluginInfo *plugin_info;
    const LilvPlugin *lilv_plugin;
    LilvInstance *instance;
    
    // Plugin properties
    char *name;
    gboolean active;
    gboolean bypass;
    
    // Audio properties
    guint n_audio_inputs;
    guint n_audio_outputs;
    guint n_control_inputs;
    guint n_control_outputs;
    
    // Port buffers
    float **audio_input_buffers;
    float **audio_output_buffers;
    float *control_input_values;
    float *control_output_values;
    
    // Audio engine reference
    ArielAudioEngine *engine;
};

G_DEFINE_FINAL_TYPE(ArielActivePlugin, ariel_active_plugin, G_TYPE_OBJECT)

static void
ariel_active_plugin_finalize(GObject *object)
{
    ArielActivePlugin *plugin = ARIEL_ACTIVE_PLUGIN(object);
    
    // Deactivate plugin
    ariel_active_plugin_deactivate(plugin);
    
    // Free instance
    if (plugin->instance) {
        lilv_instance_free(plugin->instance);
    }
    
    // Free buffers
    g_free(plugin->audio_input_buffers);
    g_free(plugin->audio_output_buffers);
    g_free(plugin->control_input_values);
    g_free(plugin->control_output_values);
    
    // Free strings
    g_free(plugin->name);
    
    // Release references
    if (plugin->plugin_info) {
        g_object_unref(plugin->plugin_info);
    }
    
    G_OBJECT_CLASS(ariel_active_plugin_parent_class)->finalize(object);
}

static void
ariel_active_plugin_class_init(ArielActivePluginClass *class)
{
    G_OBJECT_CLASS(class)->finalize = ariel_active_plugin_finalize;
}

static void
ariel_active_plugin_init(ArielActivePlugin *plugin)
{
    plugin->plugin_info = NULL;
    plugin->lilv_plugin = NULL;
    plugin->instance = NULL;
    plugin->name = NULL;
    plugin->active = FALSE;
    plugin->bypass = FALSE;
    plugin->n_audio_inputs = 0;
    plugin->n_audio_outputs = 0;
    plugin->n_control_inputs = 0;
    plugin->n_control_outputs = 0;
    plugin->audio_input_buffers = NULL;
    plugin->audio_output_buffers = NULL;
    plugin->control_input_values = NULL;
    plugin->control_output_values = NULL;
    plugin->engine = NULL;
}

ArielActivePlugin *
ariel_active_plugin_new(ArielPluginInfo *plugin_info, ArielAudioEngine *engine)
{
    if (!plugin_info || !engine) return NULL;
    
    ArielActivePlugin *plugin = g_object_new(ARIEL_TYPE_ACTIVE_PLUGIN, NULL);
    
    plugin->plugin_info = g_object_ref(plugin_info);
    plugin->engine = engine;
    plugin->name = g_strdup(ariel_plugin_info_get_name(plugin_info));
    
    // Get the lilv plugin from plugin_info - we need to access this via the original plugin manager
    // For now, we'll store it in the plugin_info structure during creation
    plugin->lilv_plugin = ariel_plugin_info_get_plugin(plugin_info);
    
    if (!plugin->lilv_plugin) {
        g_warning("Failed to get lilv plugin from plugin info");
        g_object_unref(plugin);
        return NULL;
    }
    
    // For now, assume stereo I/O for simplicity
    // TODO: Implement proper port introspection with world context
    plugin->n_audio_inputs = 2;
    plugin->n_audio_outputs = 2;
    plugin->n_control_inputs = 0;
    plugin->n_control_outputs = 0;
    
    g_print("Plugin %s: %u audio inputs, %u audio outputs, %u control inputs, %u control outputs\n",
            plugin->name, plugin->n_audio_inputs, plugin->n_audio_outputs,
            plugin->n_control_inputs, plugin->n_control_outputs);
    
    // Allocate port arrays
    if (plugin->n_audio_inputs > 0) {
        plugin->audio_input_buffers = g_malloc0(sizeof(float*) * plugin->n_audio_inputs);
    }
    if (plugin->n_audio_outputs > 0) {
        plugin->audio_output_buffers = g_malloc0(sizeof(float*) * plugin->n_audio_outputs);
    }
    if (plugin->n_control_inputs > 0) {
        plugin->control_input_values = g_malloc0(sizeof(float) * plugin->n_control_inputs);
    }
    if (plugin->n_control_outputs > 0) {
        plugin->control_output_values = g_malloc0(sizeof(float) * plugin->n_control_outputs);
    }
    
    // Create plugin instance
    plugin->instance = lilv_plugin_instantiate(plugin->lilv_plugin, engine->sample_rate, NULL);
    if (!plugin->instance) {
        g_warning("Failed to instantiate plugin %s", plugin->name);
        g_object_unref(plugin);
        return NULL;
    }
    
    // Port connection will be implemented later with proper lilv world context
    // For now, audio ports will be connected dynamically during processing
    
    g_print("Created active plugin: %s\n", plugin->name);
    
    return plugin;
}

void
ariel_active_plugin_process(ArielActivePlugin *plugin, jack_nframes_t nframes)
{
    if (!plugin || !plugin->instance || !plugin->active || plugin->bypass) {
        return;
    }
    
    // Audio ports are connected dynamically in the JACK callback
    // This function just runs the plugin
    lilv_instance_run(plugin->instance, nframes);
}

void
ariel_active_plugin_activate(ArielActivePlugin *plugin)
{
    if (!plugin || !plugin->instance) return;
    
    if (!plugin->active) {
        lilv_instance_activate(plugin->instance);
        plugin->active = TRUE;
        g_print("Activated plugin: %s\n", plugin->name);
    }
}

void
ariel_active_plugin_deactivate(ArielActivePlugin *plugin)
{
    if (!plugin || !plugin->instance) return;
    
    if (plugin->active) {
        lilv_instance_deactivate(plugin->instance);
        plugin->active = FALSE;
        g_print("Deactivated plugin: %s\n", plugin->name);
    }
}

const char *
ariel_active_plugin_get_name(ArielActivePlugin *plugin)
{
    return plugin ? plugin->name : NULL;
}

gboolean
ariel_active_plugin_is_active(ArielActivePlugin *plugin)
{
    return plugin ? plugin->active : FALSE;
}

// Helper function to connect audio ports (called from JACK callback)
void
ariel_active_plugin_connect_audio_ports(ArielActivePlugin *plugin, 
                                        float **input_buffers, 
                                        float **output_buffers)
{
    if (!plugin || !plugin->instance) return;
    
    // For now, assume first two ports are stereo input, next two are stereo output
    // TODO: Implement proper port introspection
    if (input_buffers) {
        lilv_instance_connect_port(plugin->instance, 0, input_buffers[0]);  // Left input
        lilv_instance_connect_port(plugin->instance, 1, input_buffers[1]);  // Right input
    }
    
    if (output_buffers) {
        lilv_instance_connect_port(plugin->instance, 2, output_buffers[0]); // Left output
        lilv_instance_connect_port(plugin->instance, 3, output_buffers[1]); // Right output
    }
}