#include "ariel.h"
#include <string.h>

ArielAudioEngine *
ariel_audio_engine_new(void)
{
    ArielAudioEngine *engine = g_malloc0(sizeof(ArielAudioEngine));
    if (!engine) {
        ARIEL_ERROR("Failed to allocate memory for audio engine");
        return NULL;
    }
    
    engine->active = FALSE;
    engine->sample_rate = 44100.0f;
    engine->buffer_size = 1024;
    engine->plugin_manager = NULL;
    engine->client = NULL;
    
    // Initialize port arrays to NULL
    for (int i = 0; i < 2; i++) {
        engine->input_ports[i] = NULL;
        engine->output_ports[i] = NULL;
    }
    
    ARIEL_INFO("Audio engine created successfully");
    return engine;
}

gboolean
ariel_audio_engine_start(ArielAudioEngine *engine)
{
    jack_status_t status;
    
    if (engine->active) {
        return TRUE; // Already active
    }
    
    // Open JACK client
    engine->client = jack_client_open("ariel", JackNullOption, &status);
    if (!engine->client) {
        g_warning("Failed to open JACK client: %d", status);
        return FALSE;
    }
    
    // Get sample rate and buffer size from JACK
    engine->sample_rate = (gfloat)jack_get_sample_rate(engine->client);
    engine->buffer_size = jack_get_buffer_size(engine->client);
    
    g_print("JACK: Sample rate = %.0f Hz, Buffer size = %d frames\n",
            engine->sample_rate, engine->buffer_size);
    
    // Set callbacks
    jack_set_process_callback(engine->client, ariel_jack_process_callback, engine);
    jack_on_shutdown(engine->client, ariel_jack_shutdown_callback, engine);
    
    // Create input ports
    engine->input_ports[0] = jack_port_register(engine->client, "input_L",
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsInput, 0);
    engine->input_ports[1] = jack_port_register(engine->client, "input_R",
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsInput, 0);
    
    // Create output ports
    engine->output_ports[0] = jack_port_register(engine->client, "output_L",
                                                 JACK_DEFAULT_AUDIO_TYPE,
                                                 JackPortIsOutput, 0);
    engine->output_ports[1] = jack_port_register(engine->client, "output_R",
                                                 JACK_DEFAULT_AUDIO_TYPE,
                                                 JackPortIsOutput, 0);
    
    if (!engine->input_ports[0] || !engine->input_ports[1] ||
        !engine->output_ports[0] || !engine->output_ports[1]) {
        g_warning("Failed to register JACK ports");
        jack_client_close(engine->client);
        engine->client = NULL;
        return FALSE;
    }
    
    // Activate client
    if (jack_activate(engine->client)) {
        g_warning("Failed to activate JACK client");
        jack_client_close(engine->client);
        engine->client = NULL;
        return FALSE;
    }
    
    engine->active = TRUE;
    g_print("Audio engine started successfully\n");
    
    return TRUE;
}

void
ariel_audio_engine_stop(ArielAudioEngine *engine)
{
    if (!engine->active || !engine->client) {
        return;
    }
    
    jack_client_close(engine->client);
    engine->client = NULL;
    engine->active = FALSE;
    
    g_print("Audio engine stopped\n");
}

void
ariel_audio_engine_free(ArielAudioEngine *engine)
{
    if (!engine) return;
    
    if (engine->active) {
        ariel_audio_engine_stop(engine);
    }
    
    g_free(engine);
}

void
ariel_audio_engine_set_plugin_manager(ArielAudioEngine *engine, ArielPluginManager *manager)
{
    if (!engine) {
        ARIEL_ERROR("Audio engine is NULL in set_plugin_manager");
        return;
    }
    
    if (!manager) {
        ARIEL_WARN("Plugin manager is NULL in set_plugin_manager");
    }
    
    engine->plugin_manager = manager;
    ARIEL_INFO("Plugin manager set for audio engine");
}