#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <objbase.h>
#endif

// Minimal test application to isolate the crash
typedef struct {
    GtkApplication parent;
} TestApp;

typedef struct {
    GtkApplicationClass parent_class;
} TestAppClass;

#define TEST_TYPE_APP (test_app_get_type())
G_DECLARE_FINAL_TYPE(TestApp, test_app, TEST, APP, GtkApplication)

G_DEFINE_FINAL_TYPE(TestApp, test_app, GTK_TYPE_APPLICATION)

static void
test_app_init(TestApp *app)
{
    g_print("TestApp initialized successfully\n");
}

static void
test_app_activate(GApplication *app)
{
    g_print("TestApp activated successfully\n");
    
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Minimal Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    
    GtkWidget *label = gtk_label_new("If you see this, GTK4 is working!");
    gtk_window_set_child(GTK_WINDOW(window), label);
    
    gtk_window_present(GTK_WINDOW(window));
    g_print("Test window created and shown\n");
}

static void
test_app_class_init(TestAppClass *class)
{
    G_APPLICATION_CLASS(class)->activate = test_app_activate;
}

TestApp *
test_app_new(void)
{
    return g_object_new(TEST_TYPE_APP,
                       "application-id", "com.test.minimal",
                       "flags", G_APPLICATION_DEFAULT_FLAGS,
                       NULL);
}

int main(int argc, char *argv[])
{
    g_print("=== MINIMAL TEST STARTING ===\n");
    
#ifdef G_OS_WIN32
    g_print("Running on Windows platform\n");
    
    // Allocate console for debugging
    if (AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        g_print("Windows console allocated\n");
    }
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        g_print("Failed to initialize COM: 0x%lx\n", hr);
    } else {
        g_print("COM initialized successfully\n");
    }
#endif

    g_print("Testing basic GLib functions...\n");
    
    // Test basic memory allocation
    void *test_ptr = g_malloc0(1024);
    if (!test_ptr) {
        g_print("ERROR: g_malloc0 failed\n");
        return 1;
    }
    g_free(test_ptr);
    g_print("Basic memory allocation: OK\n");
    
    // Test GLib string functions
    gchar *test_str = g_strdup("Hello, World!");
    if (!test_str) {
        g_print("ERROR: g_strdup failed\n");
        return 1;
    }
    g_print("String test: %s\n", test_str);
    g_free(test_str);
    
    g_print("Creating test application...\n");
    TestApp *app = test_app_new();
    if (!app) {
        g_print("ERROR: Failed to create test app\n");
        return 1;
    }
    g_print("Test app created successfully\n");
    
    g_print("Running test application...\n");
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_print("Application run completed with status: %d\n", status);
    
    g_object_unref(app);
    g_print("Test app cleaned up\n");
    
#ifdef G_OS_WIN32
    CoUninitialize();
    g_print("COM cleanup completed\n");
#endif
    
    g_print("=== MINIMAL TEST COMPLETED ===\n");
    return status;
}