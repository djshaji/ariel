#include "ariel.h"
#ifdef _WIN32
#include <windows.h>
#include <stdint.h>
#endif

ArielWindow *
ariel_window_new(ArielApp *app)
{
    ArielWindow *window;
    
#ifdef _WIN32
    // Windows-specific validation
    if (!app) {
        g_critical("ariel_window_new called with NULL app");
        return NULL;
    }
    
    if (!ARIEL_IS_APP(app)) {
        g_critical("ariel_window_new called with invalid ArielApp object");
        return NULL;
    }
    
    g_print("Creating window with validated ArielApp: %p\n", app);
#endif
    
    window = g_object_new(GTK_TYPE_APPLICATION_WINDOW,
                          "application", app,
                          "title", APP,
                          "default-width", 1200,
                          "default-height", 800,
                          NULL);
    
    if (!window) {
        g_critical("Failed to create application window");
        return NULL;
    }
                          
    window->app = app;
    
#ifdef _WIN32
    // Verify the assignment worked
    if (window->app != app) {
        g_critical("Window app assignment failed");
        g_object_unref(window);
        return NULL;
    }
    
    g_print("Window app assignment verified: %p -> %p\n", (void*)window, (void*)window->app);
    
    // Add reference to prevent app from being freed
    g_object_ref(app);
    g_print("Added reference to ArielApp before UI setup\n");
#endif
    
    g_print("About to call ariel_window_setup_ui with window->app = %p\n", (void*)window->app);
    ariel_window_setup_ui(window);
    g_print("Completed ariel_window_setup_ui with window->app = %p\n", (void*)window->app);

    gtk_window_maximize(GTK_WINDOW(window));
    return window;
}

void
ariel_window_setup_ui(ArielWindow *window)
{
#ifdef _WIN32
    g_print("ariel_window_setup_ui: Starting with window->app = %p\n", (void*)window->app);
    if (!window->app) {
        g_critical("ariel_window_setup_ui: window->app is NULL at start!");
        return;
    }
    if (!ARIEL_IS_APP(window->app)) {
        g_critical("ariel_window_setup_ui: window->app is not a valid ArielApp!");
        return;
    }
    
    // Additional memory check before UI creation
    g_print("Checking memory before creating UI components...\n");
    void *test_alloc = g_try_malloc(1024);
    if (test_alloc) {
        g_print("Test allocation successful\n");
        g_free(test_alloc);
    } else {
        g_critical("Test allocation failed - memory issues detected!");
        return;
    }
#endif
    
    GtkWidget *vbox;
    
#ifdef _WIN32
    g_print("STEP 1: About to create main vertical box, window->app = %p\n", (void*)window->app);
#endif
    // Create main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#ifdef _WIN32
    g_print("STEP 2: Created vbox, about to set child, window->app = %p\n", (void*)window->app);
#endif
    gtk_window_set_child(GTK_WINDOW(window), vbox);
#ifdef _WIN32
    g_print("STEP 3: Set window child, about to create header bar, window->app = %p\n", (void*)window->app);
#endif
    
    // Create header bar
    window->header_bar = ariel_create_header_bar(window);
#ifdef _WIN32
    g_print("STEP 4: Created header bar, about to set titlebar, window->app = %p\n", (void*)window->app);
#endif
    gtk_window_set_titlebar(GTK_WINDOW(window), window->header_bar);
#ifdef _WIN32
    g_print("STEP 5: Set titlebar, about to create transport, window->app = %p\n", (void*)window->app);
#endif
    
    // Create transport controls
    window->transport_box = ariel_create_transport(window);
#ifdef _WIN32
    g_print("STEP 6: Created transport, about to create paned, window->app = %p\n", (void*)window->app);
#endif
    // gtk_box_append(GTK_BOX(vbox), window->transport_box);
    
    // Create main paned view
#ifdef _WIN32
    // WORKAROUND: Store app pointer before GTK operations that corrupt memory
    ArielApp *saved_app = window->app;
    g_print("CORRUPTION WORKAROUND: Saved app pointer %p before gtk_paned_new\n", (void*)saved_app);
#endif
    window->main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#ifdef _WIN32
    g_print("STEP 7: Created paned, window->app = %p (should be %p)\n", (void*)window->app, (void*)saved_app);
    if (window->app != saved_app) {
        g_print("CORRUPTION DETECTED: Restoring window->app from %p to %p\n", (void*)window->app, (void*)saved_app);
        window->app = saved_app;
    }
#endif
    gtk_widget_set_vexpand(window->main_paned, TRUE);
#ifdef _WIN32
    g_print("STEP 8: Set expand, window->app = %p\n", (void*)window->app);
    // Verify app pointer is still correct
    if (window->app != saved_app) {
        g_print("CORRUPTION DETECTED AGAIN: Restoring window->app from %p to %p\n", (void*)window->app, (void*)saved_app);
        window->app = saved_app;
    }
#endif
    gtk_box_append(GTK_BOX(vbox), window->main_paned);
#ifdef _WIN32
    g_print("STEP 9: Completed UI setup, window->app = %p\n", (void*)window->app);
    // Final verification and restore if needed
    if (window->app != saved_app) {
        g_print("FINAL CORRUPTION FIX: Restoring window->app from %p to %p\n", (void*)window->app, (void*)saved_app);
        window->app = saved_app;
    }
#endif
    
    // Create plugin list (left pane)
#ifdef _WIN32
    g_print("About to create plugin list - validating window and app pointers\n");
    g_print("  window = %p\n", (void*)window);
    g_print("  &window->app = %p\n", (void*)&window->app);
    g_print("  window->app = %p\n", (void*)window->app);
    
    // Validate window pointer is still good
    if (IsBadReadPtr(window, sizeof(ArielWindow))) {
        g_critical("window pointer %p is not readable before plugin list creation!", (void*)window);
        return;
    }
    
    if (!window->app) {
        g_critical("window->app is NULL before plugin list creation!");
        return;
    }
    
    // Validate app pointer
    if (IsBadReadPtr(window->app, 64)) {
        g_critical("window->app pointer %p is not readable before plugin list creation!", (void*)window->app);
        return;
    }
    
    if (!ARIEL_IS_APP(window->app)) {
        g_critical("window->app is not a valid ArielApp before plugin list creation!");
        return;
    }
    
    // Store original pointer for comparison
    ArielApp *original_app_ptr = window->app;
    g_print("Stored original app pointer: %p\n", (void*)original_app_ptr);
    g_print("Calling ariel_create_plugin_list(window) where window = %p\n", (void*)window);
#endif
    window->plugin_list = ariel_create_plugin_list(window);
#ifdef _WIN32
    g_print("Created plugin list, window->app = %p\n", (void*)window->app);
    if (window->app != original_app_ptr) {
        g_critical("MEMORY CORRUPTION DETECTED! window->app changed from %p to %p during plugin list creation!", 
                   (void*)original_app_ptr, (void*)window->app);
    }
#endif
    gtk_paned_set_start_child(GTK_PANED(window->main_paned), window->plugin_list);
    
    // Create right pane with active plugins and mixer
    GtkWidget *right_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_end_child(GTK_PANED(window->main_paned), right_paned);   

    // Active plugins list (top right)
    window->active_plugins = ariel_create_active_plugins_view(window);
    gtk_paned_set_start_child(GTK_PANED(right_paned), window->active_plugins);
    gtk_widget_set_name(window->active_plugins, "active-plugins-view");
    
    // Mixer (bottom right)
    window->mixer_box = ariel_create_mixer(window);
    // gtk_paned_set_end_child(GTK_PANED(right_paned), window->mixer_box);
    
    // Set paned positions
    gtk_paned_set_position(GTK_PANED(window->main_paned), 400);
    gtk_paned_set_position(GTK_PANED(right_paned), 400);
    
    // Auto-start the audio engine
    ArielAudioEngine *engine = ariel_app_get_audio_engine(window->app);
    if (ariel_audio_engine_start(engine)) {
        gtk_button_set_label(GTK_BUTTON(window->audio_toggle), "Audio: ON");
        gtk_widget_add_css_class(GTK_WIDGET(window->audio_toggle), "suggested-action");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window->audio_toggle), TRUE);
        g_print("Audio engine auto-started successfully\n");
    } else {
        g_warning("Failed to auto-start audio engine");
    }
}