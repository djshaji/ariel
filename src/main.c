#include "ariel.h"
#include <stdio.h>
#include <stdlib.h>

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
    // Initialize audio engine and plugin manager
    app->audio_engine = ariel_audio_engine_new();
    app->plugin_manager = ariel_plugin_manager_new();// Connect plugin manager to audio engine for real-time processing\n    
    ariel_audio_engine_set_plugin_manager(app->audio_engine, app->plugin_manager);
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
        ariel_plugin_manager_free(app->plugin_manager);
        app->plugin_manager = NULL;
    }
    
    G_OBJECT_CLASS(ariel_app_parent_class)->dispose(object);
}

static void
ariel_app_activate(GApplication *application)
{
    ArielApp *app = ARIEL_APP(application);
    ArielWindow *window;
    
    // Load custom CSS if available
    ariel_load_custom_css();
    
    window = ariel_window_new(app);
    gtk_window_present(GTK_WINDOW(window));
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
    return g_object_new(ARIEL_TYPE_APP,
                       "application-id", ARIEL_APP_ID,
                       "flags", G_APPLICATION_DEFAULT_FLAGS,
                       NULL);
}

ArielAudioEngine *
ariel_app_get_audio_engine(ArielApp *app)
{
    g_return_val_if_fail(ARIEL_IS_APP(app), NULL);
    return app->audio_engine;
}

ArielPluginManager *
ariel_app_get_plugin_manager(ArielApp *app)
{
    g_return_val_if_fail(ARIEL_IS_APP(app), NULL);
    return app->plugin_manager;
}

void
ariel_load_custom_css(void)
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) {
        ARIEL_ERROR("Failed to get default display for CSS loading");
        return;
    }
    
    // First, load the default theme CSS
    GtkCssProvider *theme_provider = gtk_css_provider_new();
    
    // Try to find the installed theme file
    const char *data_dirs[] = {
        "/usr/share/ariel",
        "/usr/local/share/ariel", 
        "data",  // Development fallback
        NULL
    };
    
    gboolean theme_loaded = FALSE;
    for (int i = 0; data_dirs[i] != NULL; i++) {
        char *theme_path = g_build_filename(data_dirs[i], "ariel-theme.css", NULL);
        
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
    
    app = ariel_app_new();
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}