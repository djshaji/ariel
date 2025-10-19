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
    
    // For now, just pass through input to output
    // TODO: Process active plugins here
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