#include "ariel.h"
#include <string.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/patch/patch.h>

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
    
    // Atom port support
    guint n_atom_inputs;
    guint n_atom_outputs;
    uint32_t *atom_input_port_indices;
    uint32_t *atom_output_port_indices;
    void **atom_input_buffers;
    void **atom_output_buffers;
    uint32_t atom_buffer_size;
    
    // URIDs for Atom messaging
    LV2_URID_Map *urid_map;
    LV2_URID atom_Path;
    LV2_URID atom_Object ;
    LV2_URID atom_String;
    LV2_URID atom_Sequence;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID plugin_model_uri;
    
    // Audio engine reference
    ArielAudioEngine *engine;
    
    // UI communication
    GAsyncQueue *ui_messages;
};

// UI message structure for thread-safe communication
typedef struct {
    LV2_URID property;
    LV2_URID type;
    uint32_t size;
    char data[];
} ArielUIMessage;

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
    g_free(plugin->control_input_values);    //g_free(plugin->control_output_values);\n    \n    // Free port index arrays\n    g_free(plugin->audio_input_port_indices);\n    g_free(plugin->audio_output_port_indices);\n    g_free(plugin->control_input_port_indices);\n    g_free(plugin->control_output_port_indices);\r
    g_free(plugin->control_output_values);

    // Free port index arrays
    g_free(plugin->audio_input_port_indices);
    g_free(plugin->audio_output_port_indices);
    g_free(plugin->control_input_port_indices);
    g_free(plugin->control_output_port_indices);
    // Free Atom port buffers
    if (plugin->atom_input_buffers) {
        for (guint i = 0; i < plugin->n_atom_inputs; i++) {
            g_free(plugin->atom_input_buffers[i]);
        }
        g_free(plugin->atom_input_buffers);
    }
    if (plugin->atom_output_buffers) {
        for (guint i = 0; i < plugin->n_atom_outputs; i++) {
            g_free(plugin->atom_output_buffers[i]);
        }
        g_free(plugin->atom_output_buffers);
    }
    g_free(plugin->atom_input_port_indices);
    g_free(plugin->atom_output_port_indices);

    // Free strings
    g_free(plugin->name);
    
    // Clean up UI message queue
    if (plugin->ui_messages) {
        ArielUIMessage *msg;
        while ((msg = g_async_queue_try_pop(plugin->ui_messages)) != NULL) {
            g_free(msg);
        }
        g_async_queue_unref(plugin->ui_messages);
    }
    
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
    plugin->n_atom_inputs = 0;
    plugin->n_atom_outputs = 0;
    plugin->audio_input_buffers = NULL;
    plugin->audio_output_buffers = NULL;
    plugin->control_input_values = NULL;
    plugin->control_output_values = NULL;
    plugin->atom_input_port_indices = NULL;
    plugin->atom_output_port_indices = NULL;
    plugin->atom_input_buffers = NULL;
    plugin->atom_output_buffers = NULL;
    plugin->engine = NULL;
    plugin->ui_messages = g_async_queue_new();
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
    LilvNode *atom_port_uri = lilv_new_uri(world, LV2_ATOM__AtomPort);
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
        } else if (lilv_port_is_a(plugin->lilv_plugin, port, atom_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                plugin->n_atom_inputs++;
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                plugin->n_atom_outputs++;
            }
        }
    }
    
    // Free URI nodes
    lilv_node_free(audio_port_uri);
    lilv_node_free(control_port_uri);
    lilv_node_free(atom_port_uri);
    lilv_node_free(input_port_uri);
    lilv_node_free(output_port_uri);
    
    g_print("Plugin %s: %u audio inputs, %u audio outputs, %u control inputs, %u control outputs, %u atom inputs, %u atom outputs\n", 
            plugin->name, plugin->n_audio_inputs, plugin->n_audio_outputs,
            plugin->n_control_inputs, plugin->n_control_outputs,
            plugin->n_atom_inputs, plugin->n_atom_outputs);
    
    // Allocate port index arrays (with safety checks for zero counts)
    plugin->audio_input_port_indices = plugin->n_audio_inputs > 0 ? 
        g_malloc0(plugin->n_audio_inputs * sizeof(uint32_t)) : NULL;
    plugin->audio_output_port_indices = plugin->n_audio_outputs > 0 ? 
        g_malloc0(plugin->n_audio_outputs * sizeof(uint32_t)) : NULL;
    plugin->control_input_port_indices = plugin->n_control_inputs > 0 ? 
        g_malloc0(plugin->n_control_inputs * sizeof(uint32_t)) : NULL;
    plugin->control_output_port_indices = plugin->n_control_outputs > 0 ? 
        g_malloc0(plugin->n_control_outputs * sizeof(uint32_t)) : NULL;
    plugin->atom_input_port_indices = plugin->n_atom_inputs > 0 ? 
        g_malloc0(plugin->n_atom_inputs * sizeof(uint32_t)) : NULL;
    plugin->atom_output_port_indices = plugin->n_atom_outputs > 0 ? 
        g_malloc0(plugin->n_atom_outputs * sizeof(uint32_t)) : NULL;
    
    // Populate port index arrays
    uint32_t audio_in_idx = 0, audio_out_idx = 0;
    uint32_t control_in_idx = 0, control_out_idx = 0;
    uint32_t atom_in_idx = 0, atom_out_idx = 0;
    
    // Re-create URI nodes for second pass
    audio_port_uri = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
    control_port_uri = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    atom_port_uri = lilv_new_uri(world, LV2_ATOM__AtomPort);
    input_port_uri = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    output_port_uri = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
    
    for (uint32_t i = 0; i < num_ports; i++) {
        const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
        
        if (lilv_port_is_a(plugin->lilv_plugin, port, audio_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                if (audio_in_idx < plugin->n_audio_inputs) {
                    plugin->audio_input_port_indices[audio_in_idx++] = i;
                }
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                if (audio_out_idx < plugin->n_audio_outputs) {
                    plugin->audio_output_port_indices[audio_out_idx++] = i;
                }
            }
        } else if (lilv_port_is_a(plugin->lilv_plugin, port, control_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                if (control_in_idx < plugin->n_control_inputs) {
                    plugin->control_input_port_indices[control_in_idx++] = i;
                }
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                if (control_out_idx < plugin->n_control_outputs) {
                    plugin->control_output_port_indices[control_out_idx++] = i;
                }
            }
        } else if (lilv_port_is_a(plugin->lilv_plugin, port, atom_port_uri)) {
            if (lilv_port_is_a(plugin->lilv_plugin, port, input_port_uri)) {
                if (atom_in_idx < plugin->n_atom_inputs) {
                    plugin->atom_input_port_indices[atom_in_idx++] = i;
                }
            } else if (lilv_port_is_a(plugin->lilv_plugin, port, output_port_uri)) {
                if (atom_out_idx < plugin->n_atom_outputs) {
                    plugin->atom_output_port_indices[atom_out_idx++] = i;
                }
            }
        }
    }

    // Free URI nodes again
    lilv_node_free(audio_port_uri);
    lilv_node_free(control_port_uri);
    lilv_node_free(atom_port_uri);
    lilv_node_free(input_port_uri);
    lilv_node_free(output_port_uri);

    // Initialize control port values (with safety checks for zero counts)
    plugin->control_input_values = plugin->n_control_inputs > 0 ? 
        g_malloc0(plugin->n_control_inputs * sizeof(float)) : NULL;
    plugin->control_output_values = plugin->n_control_outputs > 0 ? 
        g_malloc0(plugin->n_control_outputs * sizeof(float)) : NULL;
    
    // Initialize control input values with defaults
    if (plugin->n_control_inputs > 0) {
        // Create URI nodes once for this loop
        LilvNode *control_uri = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
        LilvNode *input_uri = lilv_new_uri(world, LILV_URI_INPUT_PORT);
        
        uint32_t control_idx = 0;
        for (uint32_t i = 0; i < num_ports && control_idx < plugin->n_control_inputs; i++) {
            const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
            
            if (lilv_port_is_a(plugin->lilv_plugin, port, control_uri) &&
                lilv_port_is_a(plugin->lilv_plugin, port, input_uri)) {
                
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
        
        // Free URI nodes
        lilv_node_free(control_uri);
        lilv_node_free(input_uri);
    }

    // Get LV2 features from plugin manager
    ArielPluginManager *plugin_manager = ariel_app_get_plugin_manager(
        ARIEL_APP(g_application_get_default()));
    
    if (!plugin_manager) {
        g_warning("Failed to get plugin manager for %s", plugin->name);
        g_object_unref(plugin);
        return NULL;
    }
    
    // Create or update features with engine reference
    if (plugin_manager->features) {
        ariel_free_lv2_features(plugin_manager->features);
    }
    plugin_manager->features = ariel_create_lv2_features(plugin_manager, engine);
    
    if (!plugin_manager->features) {
        g_warning("Failed to create LV2 features for %s", plugin->name);
        g_object_unref(plugin);
        return NULL;
    }
    
    // Create plugin instance with LV2 features
    plugin->instance = lilv_plugin_instantiate(plugin->lilv_plugin, engine->sample_rate, 
                                              (const LV2_Feature* const*)plugin_manager->features);
    if (!plugin->instance) {
        g_warning("Failed to instantiate plugin %s", plugin->name);
        g_object_unref(plugin);
        return NULL;
    }

    // Connect control ports to their value arrays (with safety checks)
    if (plugin->n_control_inputs > 0 && plugin->control_input_port_indices && plugin->control_input_values) {
        for (guint i = 0; i < plugin->n_control_inputs; i++) {
            lilv_instance_connect_port(plugin->instance,
                                     plugin->control_input_port_indices[i],
                                     &plugin->control_input_values[i]);
        }
    }

    if (plugin->n_control_outputs > 0 && plugin->control_output_port_indices && plugin->control_output_values) {
        for (guint i = 0; i < plugin->n_control_outputs; i++) {
            lilv_instance_connect_port(plugin->instance,
                                     plugin->control_output_port_indices[i],
                                     &plugin->control_output_values[i]);
        }
    }

    // Atom port buffers will be allocated later with proper URID initialization

    // Initialize URIDs for Atom messaging if URID map is available
    if (manager && manager->urid_map && manager->features) {
        // Get the actual LV2_URID_Map from the features
        plugin->urid_map = NULL;
        for (int i = 0; manager->features[i]; i++) {
            if (strcmp(manager->features[i]->URI, LV2_URID__map) == 0) {
                plugin->urid_map = (LV2_URID_Map*)manager->features[i]->data;
                break;
            }
        }
        
        if (!plugin->urid_map) {
            g_warning("Could not find LV2_URID_Map in features");
            return NULL;
        }
        
        plugin->atom_Path = ariel_urid_map(manager->urid_map, LV2_ATOM__Path);
        plugin->atom_Object = ariel_urid_map(manager->urid_map, LV2_ATOM__Object);
        plugin->atom_String = ariel_urid_map(manager->urid_map, LV2_ATOM__String);
        plugin->atom_Sequence = ariel_urid_map(manager->urid_map, LV2_ATOM__Sequence);
        plugin->patch_Set = ariel_urid_map(manager->urid_map, LV2_PATCH__Set);
        plugin->patch_property = ariel_urid_map(manager->urid_map, LV2_PATCH__property);
        plugin->patch_value = ariel_urid_map(manager->urid_map, LV2_PATCH__value);
        
        // Neural Amp Modeler specific model parameter URI
        plugin->plugin_model_uri = ariel_urid_map(manager->urid_map, 
            "http://github.com/mikeoliphant/neural-amp-modeler-lv2#model");
    }
    
    // Set up Atom buffers (4KB should be sufficient for most messages)
    plugin->atom_buffer_size = 4096;
    if (plugin->n_atom_inputs > 0 && plugin->atom_input_port_indices) {
        plugin->atom_input_buffers = g_malloc0(plugin->n_atom_inputs * sizeof(void*));
        for (guint i = 0; i < plugin->n_atom_inputs; i++) {
            plugin->atom_input_buffers[i] = g_malloc0(plugin->atom_buffer_size);
            // Initialize as empty sequence
            LV2_Atom_Sequence *seq = (LV2_Atom_Sequence*)plugin->atom_input_buffers[i];
            seq->atom.type = plugin->atom_Sequence;
            seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
            seq->body.unit = 0;
            seq->body.pad = 0;
            
            // Connect Atom input port to buffer
            lilv_instance_connect_port(plugin->instance,
                                     plugin->atom_input_port_indices[i],
                                     plugin->atom_input_buffers[i]);
        }
    }
    
    if (plugin->n_atom_outputs > 0 && plugin->atom_output_port_indices) {
        plugin->atom_output_buffers = g_malloc0(plugin->n_atom_outputs * sizeof(void*));
        for (guint i = 0; i < plugin->n_atom_outputs; i++) {
            plugin->atom_output_buffers[i] = g_malloc0(plugin->atom_buffer_size);
            // Initialize as empty sequence
            LV2_Atom_Sequence *seq = (LV2_Atom_Sequence*)plugin->atom_output_buffers[i];
            seq->atom.type = plugin->atom_Sequence;
            seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
            seq->body.unit = 0;
            seq->body.pad = 0;
            
            // Connect Atom output port to buffer
            lilv_instance_connect_port(plugin->instance,
                                     plugin->atom_output_port_indices[i],
                                     plugin->atom_output_buffers[i]);
        }
    }

    // Audio ports will be connected dynamically during processing

    g_print("Created active plugin: %s\n", plugin->name);
    
    return plugin;
}

void
ariel_active_plugin_process(ArielActivePlugin *plugin, jack_nframes_t nframes)
{
    if (!plugin || !plugin->instance || !plugin->active || plugin->bypass) {
        return;
    }
    
    // Process UI messages before running plugin (jalv-style approach)
    ariel_active_plugin_process_ui_messages(plugin);
    
    // Reset Atom output buffers for each processing cycle
    if (plugin->atom_output_buffers && plugin->n_atom_outputs > 0) {
        for (guint i = 0; i < plugin->n_atom_outputs; i++) {
            if (plugin->atom_output_buffers[i]) {
                LV2_Atom_Sequence *seq = (LV2_Atom_Sequence*)plugin->atom_output_buffers[i];
                seq->atom.type = plugin->atom_Sequence;
                seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
                seq->body.unit = 0;
                seq->body.pad = 0;
            }
        }
    }
    
    // Run the plugin
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
    
    // Create URI nodes once
    LilvNode *control_uri = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    LilvNode *input_uri = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    
    for (uint32_t i = 0; i < num_ports; i++) {
        const LilvPort *port = lilv_plugin_get_port_by_index(plugin->lilv_plugin, i);
        
        if (lilv_port_is_a(plugin->lilv_plugin, port, control_uri) &&
            lilv_port_is_a(plugin->lilv_plugin, port, input_uri)) {
            
            if (control_idx == param_index) {
                // Free URI nodes before returning
                lilv_node_free(control_uri);
                lilv_node_free(input_uri);
                return i;
            }
            control_idx++;
        }
    }
    
    // Free URI nodes before fallback return
    lilv_node_free(control_uri);
    lilv_node_free(input_uri);
    
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
        for (guint i = 0; i < plugin->n_audio_inputs; i++) {
            // For mono plugins, connect left channel to mono input
            // For stereo plugins, connect both channels
            guint buffer_index = (i < 2) ? i : 1; // Use right channel for additional inputs
            lilv_instance_connect_port(plugin->instance, 
                                     plugin->audio_input_port_indices[i], 
                                     input_buffers[buffer_index]);
        }
    }
    
    // Connect audio output ports using stored indices  
    if (output_buffers && plugin->audio_output_port_indices) {
        for (guint i = 0; i < plugin->n_audio_outputs; i++) {
            // For mono plugins, connect mono output to left channel
            // For stereo plugins, connect both channels
            guint buffer_index = (i < 2) ? i : 1; // Use right channel for additional outputs
            lilv_instance_connect_port(plugin->instance, 
                                     plugin->audio_output_port_indices[i], 
                                     output_buffers[buffer_index]);
        }
    }
    
    // Atom ports are connected during plugin initialization
}

gboolean
ariel_active_plugin_is_mono(ArielActivePlugin *plugin)
{
    if (!plugin) return FALSE;
    
    // Consider mono if it has exactly 1 audio input and 1 audio output
    return (plugin->n_audio_inputs == 1 && plugin->n_audio_outputs == 1);
}

guint
ariel_active_plugin_get_n_audio_inputs(ArielActivePlugin *plugin)
{
    return plugin ? plugin->n_audio_inputs : 0;
}

guint
ariel_active_plugin_get_n_audio_outputs(ArielActivePlugin *plugin)
{
    return plugin ? plugin->n_audio_outputs : 0;
}

void
ariel_active_plugin_set_bypass(ArielActivePlugin *plugin, gboolean bypass)
{
    if (plugin) {
        plugin->bypass = bypass;
        g_print("Plugin %s bypass: %s\n", plugin->name, bypass ? "ON" : "OFF");
    }
}

gboolean
ariel_active_plugin_get_bypass(ArielActivePlugin *plugin)
{
    return plugin ? plugin->bypass : FALSE;
}

// Preset Management Functions
gboolean
ariel_active_plugin_save_preset(ArielActivePlugin *plugin, const char *preset_name, const char *preset_dir)
{
    if (!plugin || !preset_name || !preset_dir) {
        return FALSE;
    }
    
    // Create preset directory if it doesn't exist
    g_mkdir_with_parents(preset_dir, 0755);
    
    // Create preset file path
    char *preset_filename = g_strdup_printf("%s.preset", preset_name);
    char *preset_path = g_build_filename(preset_dir, preset_filename, NULL);
    g_free(preset_filename);
    
    // Create key file for preset data
    GKeyFile *preset_file = g_key_file_new();
    
    // Save plugin information
    g_key_file_set_string(preset_file, "plugin", "uri", ariel_plugin_info_get_uri(plugin->plugin_info));
    g_key_file_set_string(preset_file, "plugin", "name", plugin->name);
    g_key_file_set_boolean(preset_file, "plugin", "bypass", plugin->bypass);
    
    // Save parameter values
    g_key_file_set_integer(preset_file, "parameters", "count", plugin->n_control_inputs);
    
    for (guint i = 0; i < plugin->n_control_inputs; i++) {
        char *param_key = g_strdup_printf("param_%u", i);
        g_key_file_set_double(preset_file, "parameters", param_key, plugin->control_input_values[i]);
        g_free(param_key);
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
        g_print("Saved preset '%s' for plugin %s\n", preset_name, plugin->name);
    }
    
    return success;
}

gboolean
ariel_active_plugin_load_preset(ArielActivePlugin *plugin, const char *preset_path)
{
    if (!plugin || !preset_path || !g_file_test(preset_path, G_FILE_TEST_EXISTS)) {
        return FALSE;
    }
    
    GKeyFile *preset_file = g_key_file_new();
    GError *error = NULL;
    
    // Load preset file
    if (!g_key_file_load_from_file(preset_file, preset_path, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to load preset file %s: %s", preset_path, error->message);
        g_error_free(error);
        g_key_file_free(preset_file);
        return FALSE;
    }
    
    // Verify plugin URI matches
    char *saved_uri = g_key_file_get_string(preset_file, "plugin", "uri", NULL);
    const char *current_uri = ariel_plugin_info_get_uri(plugin->plugin_info);
    
    if (!saved_uri || strcmp(saved_uri, current_uri) != 0) {
        g_warning("Preset plugin URI mismatch: expected %s, got %s", current_uri, saved_uri ? saved_uri : "NULL");
        g_free(saved_uri);
        g_key_file_free(preset_file);
        return FALSE;
    }
    g_free(saved_uri);
    
    // Load bypass state
    if (g_key_file_has_key(preset_file, "plugin", "bypass", NULL)) {
        gboolean bypass = g_key_file_get_boolean(preset_file, "plugin", "bypass", NULL);
        ariel_active_plugin_set_bypass(plugin, bypass);
    }
    
    // Load parameter values
    gint param_count = g_key_file_get_integer(preset_file, "parameters", "count", NULL);
    
    for (gint i = 0; i < param_count && i < (gint)plugin->n_control_inputs; i++) {
        char *param_key = g_strdup_printf("param_%d", i);
        
        if (g_key_file_has_key(preset_file, "parameters", param_key, NULL)) {
            gdouble value = g_key_file_get_double(preset_file, "parameters", param_key, NULL);
            plugin->control_input_values[i] = (float)value;
        }
        
        g_free(param_key);
    }
    
    g_key_file_free(preset_file);
    
    char *preset_name = g_path_get_basename(preset_path);
    if (g_str_has_suffix(preset_name, ".preset")) {
        preset_name[strlen(preset_name) - 7] = '\0'; // Remove .preset extension
    }
    g_print("Loaded preset '%s' for plugin %s\n", preset_name, plugin->name);
    g_free(preset_name);
    
    return TRUE;
}

char **
ariel_active_plugin_list_presets(ArielActivePlugin *plugin, const char *preset_dir)
{
    if (!plugin || !preset_dir || !g_file_test(preset_dir, G_FILE_TEST_IS_DIR)) {
        return NULL;
    }
    
    GDir *dir = g_dir_open(preset_dir, 0, NULL);
    if (!dir) {
        return NULL;
    }
    
    GPtrArray *preset_list = g_ptr_array_new();
    const char *filename;
    
    // Get plugin URI for filtering
    const char *plugin_uri = ariel_plugin_info_get_uri(plugin->plugin_info);
    
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (g_str_has_suffix(filename, ".preset")) {
            char *preset_path = g_build_filename(preset_dir, filename, NULL);
            
            // Check if preset is for this plugin
            GKeyFile *preset_file = g_key_file_new();
            if (g_key_file_load_from_file(preset_file, preset_path, G_KEY_FILE_NONE, NULL)) {
                char *saved_uri = g_key_file_get_string(preset_file, "plugin", "uri", NULL);
                
                if (saved_uri && strcmp(saved_uri, plugin_uri) == 0) {
                    // Remove .preset extension from filename
                    char *preset_name = g_strndup(filename, strlen(filename) - 7);
                    g_ptr_array_add(preset_list, preset_name);
                }
                
                g_free(saved_uri);
            }
            
            g_key_file_free(preset_file);
            g_free(preset_path);
        }
    }
    
    g_dir_close(dir);
    
    // Convert to NULL-terminated string array
    g_ptr_array_add(preset_list, NULL);
    return (char **)g_ptr_array_free(preset_list, FALSE);
}

void
ariel_active_plugin_free_preset_list(char **preset_list)
{
    if (!preset_list) return;
    
    for (int i = 0; preset_list[i] != NULL; i++) {
        g_free(preset_list[i]);
    }
    g_free(preset_list);
}

// Send file path to plugin via thread-safe UI message queue (jalv-style approach)
void
ariel_active_plugin_set_file_parameter_with_uri(ArielActivePlugin *plugin, const char *file_path, const char *parameter_uri)
{
    if (!plugin || !file_path || !parameter_uri) {
        ariel_log(ERROR, "Cannot send file parameter: plugin=%p, file_path=%s, parameter_uri=%s", 
                (void*)plugin, file_path, parameter_uri);
        return;
    }
    
    if (!plugin->urid_map) {
        ariel_log(ERROR, "No URID map available for Atom messaging");
        return;
    }
    
    // Map the parameter URI to a URID
    LV2_URID parameter_urid = plugin->urid_map->map(plugin->urid_map->handle, parameter_uri);
    if (parameter_urid == 0) {
        ariel_log(WARN, "Failed to map parameter URI to URID: %s", parameter_uri);
        return;
    }
    
    // Create UI message with file path (jalv-style thread-safe communication)
    size_t path_len = strlen(file_path);
    ArielUIMessage *msg = g_malloc(sizeof(ArielUIMessage) + path_len + 1);
    msg->property = parameter_urid;
    msg->type = plugin->atom_Path;
    msg->size = (uint32_t)path_len + 1; // Include null terminator in size for atom:Path
    memcpy(msg->data, file_path, path_len + 1); // Copy with null terminator
    
    // Queue message for processing in audio thread
    g_async_queue_push(plugin->ui_messages, msg);
    
    ariel_log(INFO, "Queued file parameter for plugin %s: %s", plugin->name, file_path);
    g_print("Neural model will be loaded: %s\n", file_path);
}

// Send file path to plugin via Atom message (backward compatibility)
void
ariel_active_plugin_set_file_parameter(ArielActivePlugin *plugin, const char *file_path)
{
    if (!plugin || !file_path) {
        g_print("Cannot send file parameter: plugin=%p, file_path=%s\n", 
                (void*)plugin, file_path);
        return;
    }
    
    // Use the Neural Amp Modeler URI for backward compatibility
    // This is the hardcoded URI for model parameter
    const char *model_uri = "http://github.com/mikeoliphant/neural-amp-modeler-lv2#model";
    
    ariel_active_plugin_set_file_parameter_with_uri(plugin, file_path, model_uri);
}

// Check if plugin supports file parameters via Atom messaging
gboolean
ariel_active_plugin_supports_file_parameters(ArielActivePlugin *plugin)
{
    if (!plugin) return FALSE;
    
    // Check if plugin has atom input ports and required URIDs for Atom messaging
    return (plugin->n_atom_inputs > 0 && 
            plugin->urid_map != NULL &&
            plugin->atom_Path != 0 &&
            plugin->patch_Set != 0 &&
            plugin->patch_property != 0 &&
            plugin->patch_value != 0);
}

// Worker Interface Implementation

// Get LV2 instance from active plugin
LilvInstance *
ariel_active_plugin_get_instance(ArielActivePlugin *plugin)
{
    if (!plugin) return NULL;
    return plugin->instance;
}

// Check if plugin has work interface
gboolean
ariel_active_plugin_has_work_interface(ArielActivePlugin *plugin)
{
    if (!plugin || !plugin->instance) return FALSE;
    
    const LV2_Worker_Interface *work_iface = 
        (const LV2_Worker_Interface*)lilv_instance_get_extension_data(plugin->instance, LV2_WORKER__interface);
    
    return (work_iface != NULL && work_iface->work != NULL);
}

// Process worker responses for this plugin
void
ariel_active_plugin_process_worker_responses(ArielActivePlugin *plugin)
{
    if (!plugin || !plugin->instance) return;
    
    // Get the worker interface from plugin
    const LV2_Worker_Interface *work_iface = 
        (const LV2_Worker_Interface*)lilv_instance_get_extension_data(plugin->instance, LV2_WORKER__interface);
    
    if (!work_iface || !work_iface->work_response) {
        ariel_log(WARN, "Plugin %s does not provide work_response interface", 
                  plugin->name ? plugin->name : "Unknown");
        return;
    }
    
    // Get plugin manager to access response queue
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    if (!manager || !manager->worker_schedule) return;
    
    ArielWorkerSchedule *worker = manager->worker_schedule;
    ArielWorkerResponse *response;
    
    // Process responses for this specific plugin
    g_mutex_lock(&worker->response_mutex);
    GList *current = worker->response_queue->head;
    
    while (current) {
        response = (ArielWorkerResponse*)current->data;
        GList *next = current->next;
        
        // Check if this response is for our plugin
        if (response->plugin == plugin) {
            // Remove from queue
            g_queue_unlink(worker->response_queue, current);
            
            // Unlock mutex before calling plugin (avoid deadlock)
            g_mutex_unlock(&worker->response_mutex);
            
            // Call plugin's work_response method
            LV2_Worker_Status status = work_iface->work_response(
                lilv_instance_get_handle(plugin->instance),
                response->size,
                response->data);
            
            if (status == LV2_WORKER_SUCCESS) {
                ariel_log(INFO, "Worker response processed successfully for plugin %s", 
                          plugin->name ? plugin->name : "Unknown");
            } else {
                ariel_log(WARN, "Worker response failed for plugin %s (status: %d)", 
                          plugin->name ? plugin->name : "Unknown", status);
            }
            
            // Cleanup response
            g_free(response->data);
            g_free(response);
            g_list_free_1(current);
            
            // Re-lock mutex for next iteration
            g_mutex_lock(&worker->response_mutex);
            current = next;
        } else {
            current = next;
        }
    }
    g_mutex_unlock(&worker->response_mutex);
}

// Process UI messages in audio thread (jalv-style communication)
void
ariel_active_plugin_process_ui_messages(ArielActivePlugin *plugin)
{
    if (!plugin || !plugin->ui_messages || plugin->n_atom_inputs == 0) {
        return;
    }
    
    ArielUIMessage *msg;
    
    // Process all queued UI messages
    while ((msg = g_async_queue_try_pop(plugin->ui_messages)) != NULL) {
        // Create patch:Set message in Atom input buffer
        if (plugin->atom_input_buffers && plugin->atom_input_buffers[0]) {
            LV2_Atom_Sequence *seq = (LV2_Atom_Sequence*)plugin->atom_input_buffers[0];
            
            // Reset sequence
            seq->atom.type = plugin->atom_Sequence;
            seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
            seq->body.unit = 0;
            seq->body.pad = 0;
            
            // Create Atom forge for message
            LV2_Atom_Forge forge;
            lv2_atom_forge_init(&forge, plugin->urid_map);
            
            uint8_t *buffer = (uint8_t*)seq + sizeof(LV2_Atom_Sequence);
            size_t buffer_size = plugin->atom_buffer_size - sizeof(LV2_Atom_Sequence);
            lv2_atom_forge_set_buffer(&forge, buffer, buffer_size);
            
            ariel_log(INFO, "Creating patch:Set message: property=%u, value='%s', size=%u", 
                      msg->property, msg->data, msg->size);
            
            // Build patch:Set message exactly like NAM plugin expects
            LV2_Atom_Forge_Frame frame;
            if (lv2_atom_forge_sequence_head(&forge, &frame, 0)) {
                // Create event with frame time 0
                lv2_atom_forge_frame_time(&forge, 0);
                
                LV2_Atom_Forge_Frame object_frame;
                if (lv2_atom_forge_object(&forge, &object_frame, 0, plugin->patch_Set)) {
                    // Add patch:property key with the parameter URID as value
                    lv2_atom_forge_key(&forge, plugin->patch_property);
                    lv2_atom_forge_urid(&forge, msg->property);
                    
                    // Add patch:value key with the file path as atom:Path
                    lv2_atom_forge_key(&forge, plugin->patch_value);
                    lv2_atom_forge_path(&forge, msg->data, msg->size);
                    
                    lv2_atom_forge_pop(&forge, &object_frame);
                }
                lv2_atom_forge_pop(&forge, &frame);
                
                // Update sequence size
                seq->atom.size = sizeof(LV2_Atom_Sequence_Body) + forge.offset;
                
                ariel_log(INFO, "Created atom sequence: total_size=%u, forge_offset=%u", 
                          seq->atom.size, (uint32_t)forge.offset);
                
                ariel_log(INFO, "Processed UI message for plugin %s: property=%u, type=%u, size=%u",
                          plugin->name, msg->property, msg->type, msg->size);
            }
        }
        
        g_free(msg);
    }
}