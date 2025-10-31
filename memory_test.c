#include "ariel.h"
#include <stdio.h>
#include <stdlib.h>

// Minimal test to reproduce memory corruption without full UI
int main() {
    printf("Starting memory corruption test\n");
    
    // Initialize GTK (minimal)
    gtk_init();
    
    // Create ArielApp
    ArielApp *app = ariel_app_new();
    if (!app) {
        printf("Failed to create ArielApp\n");
        return 1;
    }
    
    printf("ArielApp created at: %p\n", (void*)app);
    
    // Create a mock window structure
    ArielWindow *window = g_malloc0(sizeof(ArielWindow));
    window->app = app;
    
    printf("Window created at: %p, window->app = %p\n", (void*)window, (void*)window->app);
    
    // Test ariel_app_get_plugin_manager call
    printf("About to call ariel_app_get_plugin_manager...\n");
    ArielApp *app_before = window->app;
    
    ArielPluginManager *pm = ariel_app_get_plugin_manager(window->app);
    
    printf("ariel_app_get_plugin_manager returned: %p\n", (void*)pm);
    printf("window->app after call: %p (was %p)\n", (void*)window->app, (void*)app_before);
    
    if (window->app != app_before) {
        printf("MEMORY CORRUPTION DETECTED!\n");
    } else {
        printf("No memory corruption detected\n");
    }
    
    g_free(window);
    g_object_unref(app);
    
    return 0;
}