#include "ariel.h"
#ifdef _WIN32
#include <windows.h>
#include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <objbase.h>
#include <debugapi.h>
#include <processenv.h>
#include <wincon.h>
#endif

// Define the ArielApp GObject type
struct _ArielApp {
    GtkApplication parent;
    ArielAudioEngine *audio_engine;
    ArielPluginManager *plugin_manager;
};

G_DEFINE_FINAL_TYPE(ArielApp, ariel_app, GTK_TYPE_APPLICATION)

static void
ariel_app_init(ArielApp *app)
{
    g_print("Initializing ArielApp instance at %p\n", (void*)app);
    
    if (!app) {
        g_print("ERROR: ariel_app_init called with NULL app pointer\n");
        return;
    }
    
    // Initialize fields to NULL first for safety
    app->audio_engine = NULL;
    app->plugin_manager = NULL;
    
#ifdef _WIN32
    // On Windows, defer complex initialization to avoid crash during GObject construction
    g_print("Windows detected - deferring complex initialization to activation phase\n");
    return;
#endif
    
    g_print("Creating audio engine\n");
    // Initialize audio engine and plugin manager with NULL checks
    app->audio_engine = ariel_audio_engine_new();
    if (!app->audio_engine) {
        g_print("ERROR: Failed to initialize audio engine\n");
        return;
    }
    g_print("Audio engine created successfully\n");
    
    g_print("Creating plugin manager\n");
    app->plugin_manager = ariel_plugin_manager_new();
    if (!app->plugin_manager) {
        g_print("ERROR: Failed to initialize plugin manager\n");
        ariel_audio_engine_free(app->audio_engine);
        app->audio_engine = NULL;
        return;
    }
    g_print("Plugin manager created successfully\n");
    
    // Connect plugin manager to audio engine for real-time processing
    g_print("Connecting plugin manager to audio engine\n");
    ariel_audio_engine_set_plugin_manager(app->audio_engine, app->plugin_manager);
    
    g_print("ArielApp initialization completed successfully\n");
}

static void
ariel_app_dispose(GObject *object)
{
    ArielApp *app = ARIEL_APP(object);
    
    if (app->audio_engine) {
        ariel_audio_engine_free(app->audio_engine);
        app->audio_engine = NULL;
    }
    if (app->plugin_manager) {
        // this causes crash on exit sometimes        
        // ariel_plugin_manager_free(app->plugin_manager);
        app->plugin_manager = NULL;
    }
    
    G_OBJECT_CLASS(ariel_app_parent_class)->dispose(object);
}

static void
ariel_app_activate(GApplication *application)
{
    ArielApp *app = ARIEL_APP(application);
    ArielWindow *window;
    
    g_print("Activating ArielApp\n");
    
    if (!app) {
        g_print("ERROR: Application is NULL during activation\n");
        return;
    }
    
#ifdef _WIN32
    // On Windows, perform deferred initialization here
    if (!app->audio_engine || !app->plugin_manager) {
        g_print("Performing deferred Windows initialization\n");
        
        if (!app->audio_engine) {
            g_print("Creating deferred audio engine\n");
            app->audio_engine = ariel_audio_engine_new();
            if (!app->audio_engine) {
                g_print("ERROR: Failed to initialize audio engine during activation\n");
                return;
            }
        }
        
        if (!app->plugin_manager) {
            g_print("Creating deferred plugin manager\n");
            app->plugin_manager = ariel_plugin_manager_new();
            if (!app->plugin_manager) {
                g_print("ERROR: Failed to initialize plugin manager during activation\n");
                return;
            }
        }
        
        g_print("Connecting deferred components\n");
        ariel_audio_engine_set_plugin_manager(app->audio_engine, app->plugin_manager);
        g_print("Windows deferred initialization completed\n");
    }
#endif
    
    if (!app->audio_engine || !app->plugin_manager) {
        g_print("ERROR: Audio engine or plugin manager not initialized after deferred init\n");
        return;
    }
    
    // Load custom CSS if available
    g_print("Loading custom CSS\n");
    ariel_load_custom_css();
    
    g_print("Creating main window\n");
    window = ariel_window_new(app);
    if (!window) {
        g_print("ERROR: Failed to create main window\n");
        return;
    }
    
    g_print("Presenting main window\n");
    gtk_window_present(GTK_WINDOW(window));
    g_print("Application activation completed successfully\n");
}

static void
ariel_app_class_init(ArielAppClass *class)
{
    G_APPLICATION_CLASS(class)->activate = ariel_app_activate;
    G_OBJECT_CLASS(class)->dispose = ariel_app_dispose;
}

ArielApp *
ariel_app_new(void)
{
    ARIEL_INFO("Creating new ArielApp with GObject system");
    
    // Validate GObject type system is working
    GType app_type = ARIEL_TYPE_APP;
    if (app_type == G_TYPE_INVALID) {
        ARIEL_ERROR("ARIEL_TYPE_APP is invalid - GObject type system failure");
        return NULL;
    }
    
    ARIEL_INFO("ARIEL_TYPE_APP = %lu, creating GObject", (unsigned long)app_type);
    
    ArielApp *app = g_object_new(ARIEL_TYPE_APP,
                                "application-id", ARIEL_APP_ID,
                                "flags", G_APPLICATION_DEFAULT_FLAGS,
                                NULL);
    
    if (!app) {
        ARIEL_ERROR("g_object_new returned NULL for ArielApp");
        return NULL;
    }
    
    ARIEL_INFO("ArielApp created successfully at %p", (void*)app);
    return app;
}

ArielAudioEngine *
ariel_app_get_audio_engine(ArielApp *app)
{
#ifdef _WIN32
    // Windows-specific validation
    if (!app) {
        g_critical("ariel_app_get_audio_engine called with NULL app");
        return NULL;
    }
    
    // Check pointer range validity
    if ((uintptr_t)app < 0x1000 || (uintptr_t)app > 0x7FFFFFFFFFFF) {
        g_critical("ariel_app_get_audio_engine called with invalid app pointer: %p", (void*)app);
        return NULL;
    }
    
    // Use IsBadReadPtr to safely check memory accessibility
    if (IsBadReadPtr(app, sizeof(ArielApp))) {
        g_critical("ariel_app_get_audio_engine: app pointer not readable");
        return NULL;
    }
    
    // Check if it's a valid GObject
    if (!G_IS_OBJECT(app)) {
        g_critical("ariel_app_get_audio_engine: app is not a GObject");
        return NULL;
    }
#endif
    
    g_return_val_if_fail(ARIEL_IS_APP(app), NULL);
    
#ifdef _WIN32
    // Check audio_engine pointer safely
    if (IsBadReadPtr(&app->audio_engine, sizeof(void*))) {
        g_critical("ariel_app_get_audio_engine: cannot access audio_engine field");
        return NULL;
    }
    
    if (!app->audio_engine) {
        g_critical("ariel_app_get_audio_engine: audio_engine is NULL");
        return NULL;
    }
#endif
    
    return app->audio_engine;
}

ArielPluginManager *
ariel_app_get_plugin_manager(ArielApp *app)
{
#ifdef _WIN32
    g_print("ariel_app_get_plugin_manager: ENTRY with app = %p\n", (void*)app);
    
    // Windows-specific validation - check for NULL and basic pointer validity
    if (!app) {
        g_critical("ariel_app_get_plugin_manager called with NULL app");
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: app not NULL, checking range\n");
    
    // Check if pointer looks valid (not obviously corrupted)
    if ((uintptr_t)app < 0x1000 || (uintptr_t)app > 0x7FFFFFFFFFFF) {
        g_critical("ariel_app_get_plugin_manager called with invalid app pointer: %p", (void*)app);
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: app range valid, checking readability\n");
    
    // Use IsBadReadPtr to safely check memory accessibility
    if (IsBadReadPtr(app, sizeof(ArielApp))) {
        g_critical("ariel_app_get_plugin_manager: app pointer not readable");
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: app readable, checking GObject\n");
    
    // Validate GObject header is accessible
    if (!G_IS_OBJECT(app)) {
        g_critical("ariel_app_get_plugin_manager: app is not a GObject");
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: GObject valid, checking ARIEL_IS_APP\n");
#endif
    
    g_return_val_if_fail(ARIEL_IS_APP(app), NULL);
    
#ifdef _WIN32
    g_print("ariel_app_get_plugin_manager: ARIEL_IS_APP passed, checking plugin_manager field\n");
    
    // Check plugin_manager pointer safely
    if (IsBadReadPtr(&app->plugin_manager, sizeof(void*))) {
        g_critical("ariel_app_get_plugin_manager: cannot access plugin_manager field");
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: plugin_manager field accessible\n");
    
    if (!app->plugin_manager) {
        g_critical("ariel_app_get_plugin_manager: plugin_manager is NULL");
        return NULL;
    }
    
    g_print("ariel_app_get_plugin_manager: SUCCESS returning %p\n", (void*)app->plugin_manager);
#endif
    
    return app->plugin_manager;
}

void
ariel_load_custom_css(void)
{
    // Add safety check for GTK initialization
    if (!gtk_is_initialized()) {
        ARIEL_WARN("GTK not initialized yet, skipping CSS loading");
        return;
    }
    
    GdkDisplay *display = gdk_display_get_default();
    if (!display) {
        ARIEL_ERROR("Failed to get default display for CSS loading");
        return;
    }
    
    // First, load the default theme CSS
    GtkCssProvider *theme_provider = gtk_css_provider_new();
    
    // Try to find the installed theme file
    const char *data_dirs[] = {
        "/usr/share/ariel/themes",
        "/usr/local/share/ariel/themes",        
        "themes",  // Development fallback
        NULL
    };
    
    gboolean theme_loaded = FALSE;
    char * custom_css_path = ariel_load_theme_preference();
    if (custom_css_path == NULL) {
        custom_css_path = g_strdup("ariel-theme");
    }

    char *tmp = g_strdup_printf("%s.css", custom_css_path);
    g_free (custom_css_path);
    custom_css_path = tmp;
    
    ARIEL_INFO ("Loading theme CSS: %s", custom_css_path);

    for (int i = 0; data_dirs[i] != NULL; i++) {
        char *theme_path = g_build_filename(data_dirs[i], custom_css_path, NULL);
        
        if (g_file_test(theme_path, G_FILE_TEST_EXISTS)) {
            // GTK4 API - no error parameter
            gtk_css_provider_load_from_path(theme_provider, theme_path);
            
            gtk_style_context_add_provider_for_display(display,
                                                      GTK_STYLE_PROVIDER(theme_provider),
                                                      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            ARIEL_INFO("Loaded default theme CSS from: %s", theme_path);
            theme_loaded = TRUE;
            
            g_free(theme_path);
            break;
        }
        g_free(theme_path);
    }
    
    if (!theme_loaded) {
        ARIEL_WARN("Default theme CSS file not found, using system theme");
    }
    
    g_free (custom_css_path);
    g_object_unref(theme_provider);
    
    // Then, load custom user CSS if it exists
    const char *config_dir = g_get_user_config_dir();
    if (config_dir) {
        char *ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
        char *css_file_path = g_build_filename(ariel_config_dir, "style.css", NULL);
        
        if (g_file_test(css_file_path, G_FILE_TEST_EXISTS)) {
            GtkCssProvider *css_provider = gtk_css_provider_new();
            
            // GTK4 API - no error parameter
            gtk_css_provider_load_from_path(css_provider, css_file_path);
            
            gtk_style_context_add_provider_for_display(display,
                                                      GTK_STYLE_PROVIDER(css_provider),
                                                      GTK_STYLE_PROVIDER_PRIORITY_USER);
            ARIEL_INFO("Loaded custom CSS from: %s", css_file_path);
            
            g_object_unref(css_provider);
        }
        
        g_free(ariel_config_dir);
        g_free(css_file_path);
    }
}

int
main(int argc, char *argv[])
{
    ArielApp *app;
    int status;
    
#ifdef _WIN32
    BOOL console_allocated = FALSE;
#endif
    
    // Windows-specific initialization safety checks
#ifdef _WIN32
    printf("Starting Ariel on Windows platform\n");
    fflush(stdout);
    
    // For Wine/debugging, try to attach to parent console first to keep output in same terminal
    // This prevents creating a separate console window
    
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        console_allocated = TRUE;
        printf("Attached to parent console\n");
        fflush(stdout);
    } else {
        // Check if we already have a console (Wine case)
        HWND console_window = GetConsoleWindow();
        if (console_window != NULL) {
            console_allocated = TRUE;
            printf("Using existing console\n");
            fflush(stdout);
        } else {
            // Only allocate new console as last resort (native Windows GUI case)
            if (AllocConsole()) {
                console_allocated = TRUE;
                printf("New console allocated\n");
                fflush(stdout);
            }
        }
    }
    
    if (console_allocated) {
        // Only redirect if we actually need to (for AllocConsole case)
        // Skip redirection for Wine/parent console to keep output in original terminal
        HWND console_window = GetConsoleWindow();
        if (console_window != NULL) {
            // Check if this is our own allocated console vs inherited
            DWORD console_pid;
            GetWindowThreadProcessId(console_window, &console_pid);
            if (console_pid == GetCurrentProcessId()) {
                // This is our own console, redirect streams
                FILE *fp_out, *fp_err, *fp_in;
                freopen_s(&fp_out, "CONOUT$", "w", stdout);
                freopen_s(&fp_err, "CONOUT$", "w", stderr);
                freopen_s(&fp_in, "CONIN$", "r", stdin);
            }
        }
        
        // Set console title and properties
        SetConsoleTitle(TEXT("Ariel LV2 Host - Debug Console"));
        
        printf("Windows debug console configured successfully\n");
        printf("Console output test: printf working\n");
        fprintf(stderr, "Console error test: stderr working\n");
        fflush(stdout);
        fflush(stderr);
    } else {
        // Fallback: output to debug console (visible in debugger/DebugView)
        OutputDebugStringA("Ariel: Console allocation failed, using debug output\n");
    }
    
    // Initialize COM for Windows
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        printf("Failed to initialize COM: 0x%lx\n", hr);
    } else {
        printf("COM initialized successfully\n");
    }
#endif
    
    // Validate environment before creating app
    ARIEL_INFO("Validating environment before app creation");
    
    // Check if we have basic memory allocation
    void *test_ptr = g_malloc0(64);
    if (!test_ptr) {
        ARIEL_ERROR("Basic memory allocation failed - system unstable");
        return 1;
    }
    g_free(test_ptr);
    
    ARIEL_INFO("Creating ArielApp instance");
    app = ariel_app_new();
    if (!app) {
        ARIEL_ERROR("Failed to create ArielApp instance");
        return 1;
    }

#ifdef _WIN32
    g_print("ArielApp created successfully, about to run GApplication\n");
    fflush(stdout);
#endif

    ARIEL_INFO("Running GApplication");
    
#ifdef _WIN32
    g_print("Calling g_application_run...\n");
    fflush(stdout);
#endif
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
#ifdef _WIN32
    g_print("g_application_run returned: %d\n", status);
    fflush(stdout);
#endif
    
    printf("Cleaning up application\n");
    g_object_unref(app);
    
#ifdef _WIN32
    CoUninitialize();
    printf("Windows COM cleanup completed\n");
    
    // Give time for console output to be visible
    fflush(stdout);
    fflush(stderr);
    
    // Don't free console if we attached to parent process
    // Let parent handle console cleanup
    printf("Windows cleanup completed - press Enter to continue\n");
    if (console_allocated) {
        getchar(); // Wait for user input to see output
    }
#endif
    
    return status;
}