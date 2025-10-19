#include "ariel.h"
#include <string.h>

int
ariel_jack_process_callback(jack_nframes_t nframes, void *arg)
{
    ArielAudioEngine *engine = (ArielAudioEngine *)arg;
    
    // Get input and output buffers
    jack_default_audio_sample_t *input_L = 
        (jack_default_audio_sample_t *)jack_port_get_buffer(engine->input_ports[0], nframes);
    jack_default_audio_sample_t *input_R = 
        (jack_default_audio_sample_t *)jack_port_get_buffer(engine->input_ports[1], nframes);
    jack_default_audio_sample_t *output_L = 
        (jack_default_audio_sample_t *)jack_port_get_buffer(engine->output_ports[0], nframes);
    jack_default_audio_sample_t *output_R = 
        (jack_default_audio_sample_t *)jack_port_get_buffer(engine->output_ports[1], nframes);
    
    // Process active plugins in chain
    if (engine->plugin_manager && engine->plugin_manager->active_plugin_store) {
        guint n_active = g_list_model_get_n_items(G_LIST_MODEL(engine->plugin_manager->active_plugin_store));
        
        if (n_active > 0) {
            // Use input as starting point
            jack_default_audio_sample_t *current_L = input_L;
            jack_default_audio_sample_t *current_R = input_R;
            
            // Process each active plugin in series
            for (guint i = 0; i < n_active; i++) {
                ArielActivePlugin *plugin = g_list_model_get_item(
                    G_LIST_MODEL(engine->plugin_manager->active_plugin_store), i);
                
                if (plugin && ariel_active_plugin_is_active(plugin)) {
                    // Connect audio buffers to plugin
                    float *input_buffers[2] = { current_L, current_R };
                    float *output_buffers[2] = { output_L, output_R };
                    
                    ariel_active_plugin_connect_audio_ports(plugin, input_buffers, output_buffers);
                    
                    // Process this plugin
                    ariel_active_plugin_process(plugin, nframes);
                    
                    // Output becomes input for next plugin
                    current_L = output_L;
                    current_R = output_R;
                }
                
                if (plugin) g_object_unref(plugin);
            }
        } else {
            // No active plugins, pass through input to output
            if (input_L && output_L) {
                memcpy(output_L, input_L, sizeof(jack_default_audio_sample_t) * nframes);
            } else if (output_L) {
                memset(output_L, 0, sizeof(jack_default_audio_sample_t) * nframes);
            }
            
            if (input_R && output_R) {
                memcpy(output_R, input_R, sizeof(jack_default_audio_sample_t) * nframes);
            } else if (output_R) {
                memset(output_R, 0, sizeof(jack_default_audio_sample_t) * nframes);
            }
        }
    } else {
        // No plugin manager, pass through input to output
        if (input_L && output_L) {
            memcpy(output_L, input_L, sizeof(jack_default_audio_sample_t) * nframes);
        } else if (output_L) {
            memset(output_L, 0, sizeof(jack_default_audio_sample_t) * nframes);
        }
        
        if (input_R && output_R) {
            memcpy(output_R, input_R, sizeof(jack_default_audio_sample_t) * nframes);
        } else if (output_R) {
            memset(output_R, 0, sizeof(jack_default_audio_sample_t) * nframes);
        }
    }
    
    return 0;
}

void
ariel_jack_shutdown_callback(void *arg)
{
    ArielAudioEngine *engine = (ArielAudioEngine *)arg;
    
    g_warning("JACK server shutdown");
    engine->active = FALSE;
    engine->client = NULL;
}