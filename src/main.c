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
    // Get config directory
    const char *config_dir = g_get_user_config_dir();
    if (!config_dir) {
        return;
    }
    
    char *ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
    char *css_file_path = g_build_filename(ariel_config_dir, "style.css", NULL);
    
    // Check if custom CSS file exists
    if (g_file_test(css_file_path, G_FILE_TEST_EXISTS)) {
        GtkCssProvider *css_provider = gtk_css_provider_new();
        
        // Load the CSS file
        gtk_css_provider_load_from_path(css_provider, css_file_path);
        
        // Apply CSS to the default display
        GdkDisplay *display = gdk_display_get_default();
        if (display) {
            gtk_style_context_add_provider_for_display(display,
                                                      GTK_STYLE_PROVIDER(css_provider),
                                                      GTK_STYLE_PROVIDER_PRIORITY_USER);
            
            g_print("Loaded custom CSS from: %s\n", css_file_path);
        } else {
            g_warning("Failed to get default display for CSS loading");
        }
        
        g_object_unref(css_provider);
    }
    
    g_free(ariel_config_dir);
    g_free(css_file_path);
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