#include "ariel.h"

#ifdef _WIN32
#define COBJMACROS
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <combaseapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <initguid.h>

// Define the WASAPI GUIDs we need
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xC8ADBD64, 0xE71E, 0x48A0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17);

// WASAPI-specific structure for Windows audio
typedef struct {
    IMMDeviceEnumerator *device_enumerator;
    IMMDevice *input_device;
    IMMDevice *output_device;
    IAudioClient *input_client;
    IAudioClient *output_client;
    IAudioRenderClient *render_client;
    IAudioCaptureClient *capture_client;
    
    HANDLE audio_thread;
    HANDLE stop_event;
    WAVEFORMATEX *input_format;
    WAVEFORMATEX *output_format;
    
    UINT32 input_buffer_size;
    UINT32 output_buffer_size;
    
    gboolean initialized;
    gboolean running;
    
    // Audio buffers for plugin processing
    float *input_buffer_L;
    float *input_buffer_R;
    float *output_buffer_L;
    float *output_buffer_R;
    
    ArielAudioEngine *engine;
} ArielWASAPIClient;

static ArielWASAPIClient *g_wasapi_client = NULL;

// Forward declarations
static DWORD WINAPI ariel_wasapi_audio_thread(LPVOID lpParam);
static gboolean ariel_wasapi_initialize_device(IMMDevice **device, EDataFlow dataFlow, ERole role);
static gboolean ariel_wasapi_initialize_audio_client(IMMDevice *device, IAudioClient **client, WAVEFORMATEX **format, UINT32 *buffer_size, gboolean is_input);
static void ariel_wasapi_cleanup(ArielWASAPIClient *client);

static gboolean
ariel_wasapi_init_com(void)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        ARIEL_ERROR("Failed to initialize COM: 0x%08lx", hr);
        return FALSE;
    }
    return TRUE;
}

static gboolean
ariel_wasapi_create_device_enumerator(IMMDeviceEnumerator **enumerator)
{
    HRESULT hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
                                  CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                  (void**)enumerator);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to create device enumerator: 0x%08lx", hr);
        return FALSE;
    }
    return TRUE;
}

static gboolean
ariel_wasapi_initialize_device(IMMDevice **device, EDataFlow dataFlow, ERole role)
{
    HRESULT hr;
    IMMDeviceEnumerator *enumerator = g_wasapi_client->device_enumerator;
    
    hr = enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, dataFlow, role, device);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to get default audio endpoint: 0x%08lx", hr);
        return FALSE;
    }
    
    return TRUE;
}

static gboolean
ariel_wasapi_initialize_audio_client(IMMDevice *device, IAudioClient **client, WAVEFORMATEX **format, UINT32 *buffer_size, gboolean is_input)
{
    HRESULT hr;
    REFERENCE_TIME default_period, minimum_period;
    
    // Activate audio client
    hr = device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)client);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to activate audio client: 0x%08lx", hr);
        return FALSE;
    }
    
    // Get the default format
    hr = (*client)->lpVtbl->GetMixFormat(*client, format);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to get mix format: 0x%08lx", hr);
        return FALSE;
    }
    
    // Get device periods
    hr = (*client)->lpVtbl->GetDevicePeriod(*client, &default_period, &minimum_period);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to get device period: 0x%08lx", hr);
        CoTaskMemFree(*format);
        return FALSE;
    }
    
    // Use minimum period for low latency
    REFERENCE_TIME buffer_duration = minimum_period;
    
    // Initialize audio client
    DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
    hr = (*client)->lpVtbl->Initialize(*client,
                                       AUDCLNT_SHAREMODE_SHARED,
                                       stream_flags,
                                       buffer_duration,
                                       0,
                                       *format,
                                       NULL);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to initialize audio client: 0x%08lx", hr);
        CoTaskMemFree(*format);
        return FALSE;
    }
    
    // Get buffer size
    hr = (*client)->lpVtbl->GetBufferSize(*client, buffer_size);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to get buffer size: 0x%08lx", hr);
        CoTaskMemFree(*format);
        return FALSE;
    }
    
    ARIEL_INFO("WASAPI %s: Sample rate = %lu Hz, Channels = %u, Buffer size = %lu frames",
               is_input ? "Input" : "Output",
               (*format)->nSamplesPerSec,
               (*format)->nChannels,
               *buffer_size);
    
    return TRUE;
}

static DWORD WINAPI
ariel_wasapi_audio_thread(LPVOID lpParam)
{
    ArielWASAPIClient *client = (ArielWASAPIClient*)lpParam;
    HRESULT hr;
    UINT32 padding;
    UINT32 available_frames;
    BYTE *input_data = NULL;
    BYTE *output_data = NULL;
    UINT32 frames_to_read, frames_to_write;
    DWORD flags;
    
    // Set thread priority for real-time audio
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    
    // Create event for audio client
    HANDLE audio_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!audio_event) {
        ARIEL_ERROR("Failed to create audio event");
        return 1;
    }
    
    // Set event handle for input and output clients
    if (client->input_client) {
        hr = client->input_client->lpVtbl->SetEventHandle(client->input_client, audio_event);
        if (FAILED(hr)) {
            ARIEL_ERROR("Failed to set input event handle: 0x%08lx", hr);
            CloseHandle(audio_event);
            return 1;
        }
    }
    
    if (client->output_client) {
        hr = client->output_client->lpVtbl->SetEventHandle(client->output_client, audio_event);
        if (FAILED(hr)) {
            ARIEL_ERROR("Failed to set output event handle: 0x%08lx", hr);
            CloseHandle(audio_event);
            return 1;
        }
    }
    
    // Main audio processing loop
    while (client->running) {
        DWORD wait_result = WaitForMultipleObjects(2, 
            (HANDLE[]){client->stop_event, audio_event}, 
            FALSE, INFINITE);
            
        if (wait_result == WAIT_OBJECT_0) {
            // Stop event signaled
            break;
        } else if (wait_result == WAIT_OBJECT_0 + 1) {
            // Audio event signaled - process audio
            
            // Get input data if available
            if (client->capture_client) {
                hr = client->capture_client->lpVtbl->GetNextPacketSize(client->capture_client, &frames_to_read);
                if (SUCCEEDED(hr) && frames_to_read > 0) {
                    hr = client->capture_client->lpVtbl->GetBuffer(client->capture_client,
                                                                   &input_data, &frames_to_read, &flags, NULL, NULL);
                    if (SUCCEEDED(hr)) {
                        // Convert interleaved input to separate L/R buffers
                        if (client->input_format->nChannels >= 2) {
                            float *input_samples = (float*)input_data;
                            for (UINT32 i = 0; i < frames_to_read; i++) {
                                client->input_buffer_L[i] = input_samples[i * 2];
                                client->input_buffer_R[i] = input_samples[i * 2 + 1];
                            }
                        } else {
                            // Mono input - duplicate to both channels
                            float *input_samples = (float*)input_data;
                            for (UINT32 i = 0; i < frames_to_read; i++) {
                                client->input_buffer_L[i] = client->input_buffer_R[i] = input_samples[i];
                            }
                        }
                        
                        client->capture_client->lpVtbl->ReleaseBuffer(client->capture_client, frames_to_read);
                    }
                }
            } else {
                // No input - zero the input buffers
                frames_to_read = client->output_buffer_size;
                memset(client->input_buffer_L, 0, frames_to_read * sizeof(float));
                memset(client->input_buffer_R, 0, frames_to_read * sizeof(float));
            }
            
            // Process audio through plugin chain (similar to JACK callback)
            if (client->engine && client->engine->plugin_manager) {
                // Copy input to output buffers for processing
                memcpy(client->output_buffer_L, client->input_buffer_L, frames_to_read * sizeof(float));
                memcpy(client->output_buffer_R, client->input_buffer_R, frames_to_read * sizeof(float));
                
                // Process worker responses
                if (client->engine->plugin_manager->worker_schedule) {
                    ariel_worker_process_responses(client->engine->plugin_manager->worker_schedule);
                }
                
                // Process active plugins
                if (client->engine->plugin_manager->active_plugin_store) {
                    guint n_active = g_list_model_get_n_items(G_LIST_MODEL(client->engine->plugin_manager->active_plugin_store));
                    
                    for (guint i = 0; i < n_active; i++) {
                        ArielActivePlugin *plugin = g_list_model_get_item(
                            G_LIST_MODEL(client->engine->plugin_manager->active_plugin_store), i);
                        
                        if (plugin && ariel_active_plugin_is_active(plugin)) {
                            float *input_buffers[2] = { client->output_buffer_L, client->output_buffer_R };
                            float *output_buffers[2] = { client->output_buffer_L, client->output_buffer_R };
                            
                            ariel_active_plugin_connect_audio_ports(plugin, input_buffers, output_buffers);
                            ariel_active_plugin_process(plugin, frames_to_read);
                            
                            if (ariel_active_plugin_is_mono(plugin)) {
                                memcpy(client->output_buffer_R, client->output_buffer_L, frames_to_read * sizeof(float));
                            }
                        }
                        
                        if (plugin) g_object_unref(plugin);
                    }
                }
            } else {
                // No processing - pass through
                memcpy(client->output_buffer_L, client->input_buffer_L, frames_to_read * sizeof(float));
                memcpy(client->output_buffer_R, client->input_buffer_R, frames_to_read * sizeof(float));
            }
            
            // Output processed audio
            if (client->render_client) {
                hr = client->output_client->lpVtbl->GetCurrentPadding(client->output_client, &padding);
                if (SUCCEEDED(hr)) {
                    available_frames = client->output_buffer_size - padding;
                    frames_to_write = min(available_frames, frames_to_read);
                    
                    if (frames_to_write > 0) {
                        hr = client->render_client->lpVtbl->GetBuffer(client->render_client, frames_to_write, &output_data);
                        if (SUCCEEDED(hr)) {
                            // Convert separate L/R buffers to interleaved output
                            if (client->output_format->nChannels >= 2) {
                                float *output_samples = (float*)output_data;
                                for (UINT32 i = 0; i < frames_to_write; i++) {
                                    output_samples[i * 2] = client->output_buffer_L[i];
                                    output_samples[i * 2 + 1] = client->output_buffer_R[i];
                                }
                            } else {
                                // Mono output - mix L and R
                                float *output_samples = (float*)output_data;
                                for (UINT32 i = 0; i < frames_to_write; i++) {
                                    output_samples[i] = (client->output_buffer_L[i] + client->output_buffer_R[i]) * 0.5f;
                                }
                            }
                            
                            client->render_client->lpVtbl->ReleaseBuffer(client->render_client, frames_to_write, 0);
                        }
                    }
                }
            }
        }
    }
    
    CloseHandle(audio_event);
    return 0;
}

gboolean
ariel_wasapi_start(ArielAudioEngine *engine)
{
    if (g_wasapi_client && g_wasapi_client->running) {
        return TRUE; // Already running
    }
    
    if (!g_wasapi_client) {
        g_wasapi_client = g_malloc0(sizeof(ArielWASAPIClient));
        if (!g_wasapi_client) {
            ARIEL_ERROR("Failed to allocate WASAPI client");
            return FALSE;
        }
    }
    
    g_wasapi_client->engine = engine;
    
    // Initialize COM
    if (!ariel_wasapi_init_com()) {
        g_free(g_wasapi_client);
        g_wasapi_client = NULL;
        return FALSE;
    }
    
    // Create device enumerator
    if (!ariel_wasapi_create_device_enumerator(&g_wasapi_client->device_enumerator)) {
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Initialize output device and client
    if (!ariel_wasapi_initialize_device(&g_wasapi_client->output_device, eRender, eConsole)) {
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    if (!ariel_wasapi_initialize_audio_client(g_wasapi_client->output_device,
                                              &g_wasapi_client->output_client,
                                              &g_wasapi_client->output_format,
                                              &g_wasapi_client->output_buffer_size,
                                              FALSE)) {
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Get render client
    HRESULT hr = g_wasapi_client->output_client->lpVtbl->GetService(g_wasapi_client->output_client,
                                                                    &IID_IAudioRenderClient,
                                                                    (void**)&g_wasapi_client->render_client);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to get render client: 0x%08lx", hr);
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Try to initialize input device (optional)
    if (ariel_wasapi_initialize_device(&g_wasapi_client->input_device, eCapture, eConsole)) {
        if (ariel_wasapi_initialize_audio_client(g_wasapi_client->input_device,
                                                  &g_wasapi_client->input_client,
                                                  &g_wasapi_client->input_format,
                                                  &g_wasapi_client->input_buffer_size,
                                                  TRUE)) {
            // Get capture client
            hr = g_wasapi_client->input_client->lpVtbl->GetService(g_wasapi_client->input_client,
                                                                   &IID_IAudioCaptureClient,
                                                                   (void**)&g_wasapi_client->capture_client);
            if (FAILED(hr)) {
                ARIEL_WARN("Failed to get capture client: 0x%08lx", hr);
                // Continue without input
            }
        }
    }
    
    // Allocate audio buffers
    UINT32 max_buffer_size = max(g_wasapi_client->input_buffer_size, g_wasapi_client->output_buffer_size);
    g_wasapi_client->input_buffer_L = g_malloc0(max_buffer_size * sizeof(float));
    g_wasapi_client->input_buffer_R = g_malloc0(max_buffer_size * sizeof(float));
    g_wasapi_client->output_buffer_L = g_malloc0(max_buffer_size * sizeof(float));
    g_wasapi_client->output_buffer_R = g_malloc0(max_buffer_size * sizeof(float));
    
    // Create stop event
    g_wasapi_client->stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_wasapi_client->stop_event) {
        ARIEL_ERROR("Failed to create stop event");
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Start audio clients
    if (g_wasapi_client->input_client) {
        hr = g_wasapi_client->input_client->lpVtbl->Start(g_wasapi_client->input_client);
        if (FAILED(hr)) {
            ARIEL_WARN("Failed to start input client: 0x%08lx", hr);
        }
    }
    
    hr = g_wasapi_client->output_client->lpVtbl->Start(g_wasapi_client->output_client);
    if (FAILED(hr)) {
        ARIEL_ERROR("Failed to start output client: 0x%08lx", hr);
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Create and start audio thread
    g_wasapi_client->running = TRUE;
    g_wasapi_client->audio_thread = CreateThread(NULL, 0, ariel_wasapi_audio_thread, g_wasapi_client, 0, NULL);
    if (!g_wasapi_client->audio_thread) {
        ARIEL_ERROR("Failed to create audio thread");
        g_wasapi_client->running = FALSE;
        ariel_wasapi_cleanup(g_wasapi_client);
        return FALSE;
    }
    
    // Update engine parameters
    engine->sample_rate = (gfloat)g_wasapi_client->output_format->nSamplesPerSec;
    engine->buffer_size = (gint)g_wasapi_client->output_buffer_size;
    engine->active = TRUE;
    
    ARIEL_INFO("WASAPI audio engine started successfully");
    return TRUE;
}

void
ariel_wasapi_stop(ArielAudioEngine *engine)
{
    if (!g_wasapi_client || !g_wasapi_client->running) {
        return;
    }
    
    // Signal stop and wait for thread
    g_wasapi_client->running = FALSE;
    if (g_wasapi_client->stop_event) {
        SetEvent(g_wasapi_client->stop_event);
    }
    
    if (g_wasapi_client->audio_thread) {
        WaitForSingleObject(g_wasapi_client->audio_thread, 5000);
        CloseHandle(g_wasapi_client->audio_thread);
        g_wasapi_client->audio_thread = NULL;
    }
    
    ariel_wasapi_cleanup(g_wasapi_client);
    engine->active = FALSE;
    
    ARIEL_INFO("WASAPI audio engine stopped");
}

static void
ariel_wasapi_cleanup(ArielWASAPIClient *client)
{
    if (!client) return;
    
    // Stop audio clients
    if (client->input_client) {
        client->input_client->lpVtbl->Stop(client->input_client);
        client->input_client->lpVtbl->Release(client->input_client);
        client->input_client = NULL;
    }
    
    if (client->output_client) {
        client->output_client->lpVtbl->Stop(client->output_client);
        client->output_client->lpVtbl->Release(client->output_client);
        client->output_client = NULL;
    }
    
    // Release service clients
    if (client->render_client) {
        client->render_client->lpVtbl->Release(client->render_client);
        client->render_client = NULL;
    }
    
    if (client->capture_client) {
        client->capture_client->lpVtbl->Release(client->capture_client);
        client->capture_client = NULL;
    }
    
    // Release devices
    if (client->input_device) {
        client->input_device->lpVtbl->Release(client->input_device);
        client->input_device = NULL;
    }
    
    if (client->output_device) {
        client->output_device->lpVtbl->Release(client->output_device);
        client->output_device = NULL;
    }
    
    // Release device enumerator
    if (client->device_enumerator) {
        client->device_enumerator->lpVtbl->Release(client->device_enumerator);
        client->device_enumerator = NULL;
    }
    
    // Free formats
    if (client->input_format) {
        CoTaskMemFree(client->input_format);
        client->input_format = NULL;
    }
    
    if (client->output_format) {
        CoTaskMemFree(client->output_format);
        client->output_format = NULL;
    }
    
    // Free audio buffers
    if (client->input_buffer_L) {
        g_free(client->input_buffer_L);
        client->input_buffer_L = NULL;
    }
    
    if (client->input_buffer_R) {
        g_free(client->input_buffer_R);
        client->input_buffer_R = NULL;
    }
    
    if (client->output_buffer_L) {
        g_free(client->output_buffer_L);
        client->output_buffer_L = NULL;
    }
    
    if (client->output_buffer_R) {
        g_free(client->output_buffer_R);
        client->output_buffer_R = NULL;
    }
    
    // Close handles
    if (client->stop_event) {
        CloseHandle(client->stop_event);
        client->stop_event = NULL;
    }
    
    // Don't free the client itself here - it's global
    client->initialized = FALSE;
}

// Device enumeration for UI
GList *
ariel_wasapi_enumerate_devices(gboolean input_devices)
{
    GList *device_list = NULL;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDeviceCollection *device_collection = NULL;
    UINT device_count = 0;
    
    if (!ariel_wasapi_init_com()) {
        return NULL;
    }
    
    if (!ariel_wasapi_create_device_enumerator(&enumerator)) {
        return NULL;
    }
    
    HRESULT hr = enumerator->lpVtbl->EnumAudioEndpoints(enumerator,
                                                        input_devices ? eCapture : eRender,
                                                        DEVICE_STATE_ACTIVE,
                                                        &device_collection);
    if (FAILED(hr)) {
        enumerator->lpVtbl->Release(enumerator);
        return NULL;
    }
    
    hr = device_collection->lpVtbl->GetCount(device_collection, &device_count);
    if (FAILED(hr)) {
        device_collection->lpVtbl->Release(device_collection);
        enumerator->lpVtbl->Release(enumerator);
        return NULL;
    }
    
    for (UINT i = 0; i < device_count; i++) {
        IMMDevice *device = NULL;
        IPropertyStore *property_store = NULL;
        PROPVARIANT device_name;
        
        PropVariantInit(&device_name);
        
        hr = device_collection->lpVtbl->Item(device_collection, i, &device);
        if (SUCCEEDED(hr)) {
            hr = device->lpVtbl->OpenPropertyStore(device, STGM_READ, &property_store);
            if (SUCCEEDED(hr)) {
                hr = property_store->lpVtbl->GetValue(property_store, &PKEY_Device_FriendlyName, &device_name);
                if (SUCCEEDED(hr) && device_name.vt == VT_LPWSTR) {
                    // Convert wide string to UTF-8
                    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, device_name.pwszVal, -1, NULL, 0, NULL, NULL);
                    if (utf8_len > 0) {
                        char *utf8_name = g_malloc(utf8_len);
                        WideCharToMultiByte(CP_UTF8, 0, device_name.pwszVal, -1, utf8_name, utf8_len, NULL, NULL);
                        device_list = g_list_append(device_list, utf8_name);
                    }
                }
                PropVariantClear(&device_name);
                property_store->lpVtbl->Release(property_store);
            }
            device->lpVtbl->Release(device);
        }
    }
    
    device_collection->lpVtbl->Release(device_collection);
    enumerator->lpVtbl->Release(enumerator);
    
    return device_list;
}

#endif // _WIN32