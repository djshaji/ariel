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