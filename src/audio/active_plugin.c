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
    
    // Port index mappings
    uint32_t *audio_input_port_indices;
    uint32_t *audio_output_port_indices;
    uint32_t *control_input_port_indices;
    uint32_t *control_output_port_indices;
    
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
    
        // Free buffers\n    g_free(plugin->audio_input_buffers);\n    g_free(plugin->audio_output_buffers);\n    g_free(plugin->control_input_values);\n    g_free(plugin->control_output_values);\n    \n    // Free port index arrays\n    g_free(plugin->audio_input_port_indices);\n    g_free(plugin->audio_output_port_indices);\n    g_free(plugin->control_input_port_indices);\n    g_free(plugin->control_output_port_indices);
    
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
    
    // Proper port introspection
    plugin->n_audio_inputs = 0;
    plugin->n_audio_outputs = 0;
    plugin->n_control_inputs = 0;
    plugin->n_control_outputs = 0;
    
    // Get the lilv world from the plugin manager (we need world context for URI creation)
    ArielPluginManager *manager = ariel_app_get_plugin_manager(
        ARIEL_APP(g_application_get_default()));
    LilvWorld *world = manager->world;
    
    // Create URI nodes for port types
    LilvNode *audio_port_uri = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
    LilvNode *control_port_uri = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    LilvNode *input_port_uri = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    LilvNode *output_port_uri = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
    
    // Count ports by type
    const uint32_t num_ports = lilv_plugin_get_num_ports(plugin->lilv_plugin);
    for (uint32_t i = 0; i < num_ports; i++) {
        const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
        
        if (lilv_port_is_a(plugin->lilv_plugin, port, audio_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                plugin->n_audio_inputs++;
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                plugin->n_audio_outputs++;
            }
        } else if (lilv_port_is_a(plugin->lilv_plugin, port, control_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                plugin->n_control_inputs++;
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                plugin->n_control_outputs++;
            }
        }
    }
    
    // Free URI nodes
    lilv_node_free(audio_port_uri);
    lilv_node_free(control_port_uri);
    lilv_node_free(input_port_uri);
    lilv_node_free(output_port_uri);\n    \n    g_print(\"Plugin %s: %u audio inputs, %u audio outputs, %u control inputs, %u control outputs\\n\",\n            plugin->name, plugin->n_audio_inputs, plugin->n_audio_outputs,\n            plugin->n_control_inputs, plugin->n_control_outputs);\n    \n    // Allocate port index arrays\n    plugin->audio_input_port_indices = g_malloc0(plugin->n_audio_inputs * sizeof(uint32_t));\n    plugin->audio_output_port_indices = g_malloc0(plugin->n_audio_outputs * sizeof(uint32_t));\n    plugin->control_input_port_indices = g_malloc0(plugin->n_control_inputs * sizeof(uint32_t));\n    plugin->control_output_port_indices = g_malloc0(plugin->n_control_outputs * sizeof(uint32_t));\n    \n    // Populate port index arrays\n    uint32_t audio_in_idx = 0, audio_out_idx = 0;\n    uint32_t control_in_idx = 0, control_out_idx = 0;\n    \n    // Re-create URI nodes for second pass\n    audio_port_uri = lilv_new_uri(world, LILV_URI_AUDIO_PORT);\n    control_port_uri = lilv_new_uri(world, LILV_URI_CONTROL_PORT);\n    input_port_uri = lilv_new_uri(world, LILV_URI_INPUT_PORT);\n    output_port_uri = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);\n    \n    for (uint32_t i = 0; i < num_ports; i++) {\n        const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);\n        \n        if (lilv_port_is_a(plugin->lilv_plugin, port, audio_port_uri)) {\n            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {\n                if (audio_in_idx < plugin->n_audio_inputs) {\n                    plugin->audio_input_port_indices[audio_in_idx++] = i;\n                }\n            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {\n                if (audio_out_idx < plugin->n_audio_outputs) {\n                    plugin->audio_output_port_indices[audio_out_idx++] = i;\n                }\n            }\n        } else if (lilv_port_is_a(plugin->lilv_plugin, port, control_port_uri)) {\n            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {\n                if (control_in_idx < plugin->n_control_inputs) {\n                    plugin->control_input_port_indices[control_in_idx++] = i;\n                }\n            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {\n                if (control_out_idx < plugin->n_control_outputs) {\n                    plugin->control_output_port_indices[control_out_idx++] = i;\n                }\n            }\n        }\n    }\n    \n    // Free URI nodes again\n    lilv_node_free(audio_port_uri);\n    lilv_node_free(control_port_uri);\n    lilv_node_free(input_port_uri);\n    lilv_node_free(output_port_uri);\n    \n    // Initialize control port values
    plugin->control_input_values = g_malloc0(plugin->n_control_inputs * sizeof(float));
    plugin->control_output_values = g_malloc0(plugin->n_control_outputs * sizeof(float));
    
    // Initialize control input values with defaults
    if (plugin->n_control_inputs > 0) {
        uint32_t control_idx = 0;
        for (uint32_t i = 0; i < num_ports && control_idx < plugin->n_control_inputs; i++) {
            const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
            
            if (lilv_port_is_a(plugin->lilv_plugin, port, lilv_new_uri(world, LILV_URI_CONTROL_PORT)) &&
                lilv_port_is_a(plugin->lilv_plugin, port, lilv_new_uri(world, LILV_URI_INPUT_PORT))) {
                
                // Get default value
                LilvNode *default_node = NULL;
                lilv_port_get_range(plugin->lilv_plugin, port, &default_node, NULL, NULL);
                
                if (default_node) {
                    plugin->control_input_values[control_idx] = lilv_node_as_float(default_node);
                    lilv_node_free(default_node);
                } else {
                    plugin->control_input_values[control_idx] = 0.0f;
                }
                
                control_idx++;
            }
        }
    }
    
        // Create plugin instance\n    plugin->instance = lilv_plugin_instantiate(plugin->lilv_plugin, engine->sample_rate, NULL);\n    if (!plugin->instance) {\n        g_warning(\"Failed to instantiate plugin %s\", plugin->name);\n        g_object_unref(plugin);\n        return NULL;\n    }\n    \n    // Connect control ports to their value arrays\n    for (guint i = 0; i < plugin->n_control_inputs; i++) {\n        lilv_instance_connect_port(plugin->instance, \n                                 plugin->control_input_port_indices[i], \n                                 &plugin->control_input_values[i]);\n    }\n    \n    for (guint i = 0; i < plugin->n_control_outputs; i++) {\n        lilv_instance_connect_port(plugin->instance, \n                                 plugin->control_output_port_indices[i], \n                                 &plugin->control_output_values[i]);\n    }\n    \n    // Audio ports will be connected dynamically during processing
    
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
    g_return_val_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin), NULL);
    return plugin->name;
}

const LilvPlugin *
ariel_active_plugin_get_lilv_plugin(ArielActivePlugin *plugin)
{
    g_return_val_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin), NULL);
    return plugin->lilv_plugin;
}

uint32_t
ariel_active_plugin_get_num_parameters(ArielActivePlugin *plugin)
{
    g_return_val_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin), 0);
    return plugin->n_control_inputs;
}

float
ariel_active_plugin_get_parameter(ArielActivePlugin *plugin, uint32_t index)
{
    g_return_val_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin), 0.0f);
    g_return_val_if_fail(index < plugin->n_control_inputs, 0.0f);
    
    if (plugin->control_input_values) {
        return plugin->control_input_values[index];
    }
    return 0.0f;
}

void
ariel_active_plugin_set_parameter(ArielActivePlugin *plugin, uint32_t index, float value)
{
    g_return_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin));
    g_return_if_fail(index < plugin->n_control_inputs);
    
    if (plugin->control_input_values) {
        plugin->control_input_values[index] = value;
    }
}

uint32_t
ariel_active_plugin_get_control_port_index(ArielActivePlugin *plugin, uint32_t param_index)
{
    g_return_val_if_fail(ARIEL_IS_ACTIVE_PLUGIN(plugin), 0);
    g_return_val_if_fail(param_index < plugin->n_control_inputs, 0);
    
    // Find the actual port index for the nth control input
    ArielPluginManager *manager = ariel_app_get_plugin_manager(
        ARIEL_APP(g_application_get_default()));
    LilvWorld *world = manager->world;
    
    const uint32_t num_ports = lilv_plugin_get_num_ports(plugin->lilv_plugin);
    uint32_t control_idx = 0;
    
    for (uint32_t i = 0; i < num_ports; i++) {
        const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
        
        if (lilv_port_is_a(plugin->lilv_plugin, port, lilv_new_uri(world, LILV_URI_CONTROL_PORT)) &&
            lilv_port_is_a(plugin->lilv_plugin, port, lilv_new_uri(world, LILV_URI_INPUT_PORT))) {
            
            if (control_idx == param_index) {
                return i;
            }
            control_idx++;
        }
    }
    
    return 0; // Fallback
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
    
    // Connect audio input ports using stored indices
    if (input_buffers && plugin->audio_input_port_indices) {
        for (guint i = 0; i < plugin->n_audio_inputs && i < 2; i++) {
            lilv_instance_connect_port(plugin->instance, 
                                     plugin->audio_input_port_indices[i], 
                                     input_buffers[i]);
        }
    }
    
    // Connect audio output ports using stored indices  
    if (output_buffers && plugin->audio_output_port_indices) {
        for (guint i = 0; i < plugin->n_audio_outputs && i < 2; i++) {
            lilv_instance_connect_port(plugin->instance, 
                                     plugin->audio_output_port_indices[i], 
                                     output_buffers[i]);
        }
    }
}