#include "ariel.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>

typedef struct {
    ArielWindow *window;
    GtkDropDown *theme_dropdown;
    GListStore *theme_store;
} ArielSettingsData;

void ariel_set_default_theme () ;

static void
settings_data_free(ArielSettingsData *data)
{
    if (data) {
        g_object_unref(data->theme_store);
        g_free(data);
    }
}

static char **
ariel_probe_available_themes(int *count)
{
    char **themes = NULL;
    DIR *dir;
    struct dirent *entry;
    int theme_count = 0;
    int capacity = 10;
    
    // Get themes directory path
    char *themes_dir = g_build_filename(g_get_current_dir(), "themes", NULL);
    
    // Allocate initial array
    themes = g_malloc(capacity * sizeof(char*));
    
    dir = opendir(themes_dir);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            // Check if it's a CSS file
            if (g_str_has_suffix(entry->d_name, ".css")) {
                // Remove .css extension for display name
                char *theme_name = g_strndup(entry->d_name, strlen(entry->d_name) - 4);
                
                // Resize array if needed
                if (theme_count >= capacity) {
                    capacity *= 2;
                    themes = g_realloc(themes, capacity * sizeof(char*));
                }
                
                themes[theme_count] = theme_name;
                theme_count++;
            }
        }
        closedir(dir);
    }
    
    g_free(themes_dir);
    
    // Add default theme option
    if (theme_count >= capacity) {
        capacity++;
        themes = g_realloc(themes, capacity * sizeof(char*));
    }
    themes[theme_count] = g_strdup("default");
    theme_count++;
    
    *count = theme_count;
    return themes;
}

static char *
get_config_file_path(void)
{
    const char *config_dir = g_get_user_config_dir();
    char *ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
    
    // Create config directory if it doesn't exist
    g_mkdir_with_parents(ariel_config_dir, 0755);
    
    char *config_file = g_build_filename(ariel_config_dir, "config.ini", NULL);
    g_free(ariel_config_dir);
    
    return config_file;
}

void
ariel_save_theme_preference(const char *theme_name)
{
    char *config_file = get_config_file_path();
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;
    
    // Load existing config if it exists
    if (g_file_test(config_file, G_FILE_TEST_EXISTS)) {
        g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_NONE, &error);
        if (error) {
            g_warning("Failed to load config file: %s", error->message);
            g_error_free(error);
            error = NULL;
        }
    }
    
    // Set theme preference
    g_key_file_set_string(key_file, "UI", "theme", theme_name);
    
    // Save config file
    gchar *data = g_key_file_to_data(key_file, NULL, &error);
    if (data && !error) {
        g_file_set_contents(config_file, data, -1, &error);
        if (error) {
            g_warning("Failed to save config file: %s", error->message);
            g_error_free(error);
        } else {
            g_print("Saved theme preference: %s\n", theme_name);
        }
        g_free(data);
    }
    
    g_key_file_free(key_file);
    g_free(config_file);
    OUT();
}

char *
ariel_load_theme_preference(void)
{
    char *config_file = get_config_file_path();
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;
    char *theme_name = NULL;
    
    if (g_file_test(config_file, G_FILE_TEST_EXISTS)) {
        g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_NONE, &error);
        if (!error) {
            theme_name = g_key_file_get_string(key_file, "UI", "theme", &error);
            if (error) {
                // Theme preference doesn't exist, use default
                g_error_free(error);
                theme_name = g_strdup("default");
            }
        } else {
            g_warning("Failed to load config file: %s", error->message);
            g_error_free(error);
            theme_name = g_strdup("default");
        }
    } else {
        // Config file doesn't exist, use default
        theme_name = g_strdup("default");
    }
    
    g_key_file_free(key_file);
    g_free(config_file);
    
    return theme_name;
}

static void
ariel_apply_theme(const char *theme_name)
{
    GtkCssProvider *provider;
    GdkDisplay *display;
    char *css_file_path;
    GError *error = NULL;
    
    // Get the CSS provider (create if not exists)
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    
    // Remove existing custom CSS first
    gtk_style_context_remove_provider_for_display(display, GTK_STYLE_PROVIDER(provider));
    
    if (g_strcmp0(theme_name, "default") == 0) {
        // For default theme, don't load any custom CSS
        g_print("Applied default theme\n");
        g_object_unref(provider);
        return;
    }
    
    // Build path to theme CSS file
    css_file_path = g_build_filename(g_get_current_dir(), "themes", 
                                     g_strdup_printf("%s.css", theme_name), NULL);
    
    // Load CSS from file
    if (g_file_test(css_file_path, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(provider, css_file_path);
        
        // Apply CSS to display
        gtk_style_context_add_provider_for_display(display,
                                                   GTK_STYLE_PROVIDER(provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        g_print("Applied theme: %s\n", theme_name);
    } else {
        g_warning("Theme file not found: %s", css_file_path);
        ariel_set_default_theme();
    }
    
    g_free(css_file_path);
    g_object_unref(provider);
}


void
ariel_apply_saved_theme(void)
{
    char *saved_theme = ariel_load_theme_preference();
    if (saved_theme) {
        ariel_apply_theme(saved_theme);
        g_free(saved_theme);
    }
}

static void
on_theme_changed(GtkDropDown *dropdown, GParamSpec *pspec, ArielSettingsData *data)
{
    guint selected = gtk_drop_down_get_selected(dropdown);
    GtkStringObject *string_obj = g_list_model_get_item(G_LIST_MODEL(data->theme_store), selected);
    
    if (string_obj) {
        const char *theme_name = gtk_string_object_get_string(string_obj);
        ariel_apply_theme(theme_name);
        ariel_save_theme_preference(theme_name);
        g_object_unref(string_obj);
    }
}

static void
on_settings_response(GtkDialog *dialog, int response_id, ArielSettingsData *data)
{
    // Save settings here if needed
    // Only destroy the dialog here. Freeing `data` must wait until the
    // dialog is fully destroyed because GTK widgets (like the drop-down)
    // may still hold a reference to the GListStore model. Freeing it
    // immediately can cause use-after-free crashes when the widget
    // teardown continues.
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void
on_settings_destroy(GtkWidget *widget, gpointer user_data)
{
    ArielSettingsData *data = (ArielSettingsData*)user_data;
    settings_data_free(data);
}

void
ariel_show_settings_dialog(ArielWindow *window)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *grid;
    GtkWidget *theme_label;
    ArielSettingsData *data;
    char **themes;
    int theme_count;
    int i;
    
    // Create settings data
    data = g_malloc0(sizeof(ArielSettingsData));
    data->window = window;
    data->theme_store = g_list_store_new(GTK_TYPE_STRING_OBJECT);
    
    // Create dialog
    dialog = gtk_dialog_new_with_buttons("Settings",
                                        GTK_WINDOW(window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "Close", GTK_RESPONSE_CLOSE,
                                        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    
    // Get content area and set up layout
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_set_margin_start(content_area, 20);
    gtk_widget_set_margin_end(content_area, 20);
    gtk_widget_set_margin_top(content_area, 20);
    gtk_widget_set_margin_bottom(content_area, 20);
    
    // Create grid for settings
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_hexpand(grid, TRUE);
    
    // Theme selection
    theme_label = gtk_label_new("Theme:");
    gtk_widget_set_halign(theme_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), theme_label, 0, 0, 1, 1);
    
    // Populate theme store
    themes = ariel_probe_available_themes(&theme_count);
    for (i = 0; i < theme_count; i++) {
        GtkStringObject *string_obj = gtk_string_object_new(themes[i]);
        g_list_store_append(data->theme_store, string_obj);
        g_object_unref(string_obj);
    }
    
    // Create theme dropdown
    data->theme_dropdown = GTK_DROP_DOWN(gtk_drop_down_new(G_LIST_MODEL(data->theme_store), NULL));
    gtk_widget_set_hexpand(GTK_WIDGET(data->theme_dropdown), TRUE);
    
    // Set initial selection to saved preference
    char *saved_theme = ariel_load_theme_preference();
    guint initial_selection = 0;
    if (saved_theme) {
        for (i = 0; i < theme_count; i++) {
            if (g_strcmp0(themes[i], saved_theme) == 0) {
                initial_selection = i;
                break;
            }
        }
        g_free(saved_theme);
    }
    gtk_drop_down_set_selected(data->theme_dropdown, initial_selection);
    
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(data->theme_dropdown), 1, 0, 1, 1);
    
    // Connect theme change signal
    g_signal_connect(data->theme_dropdown, "notify::selected",
                     G_CALLBACK(on_theme_changed), data);
    
    // Add more settings sections here in the future
    GtkWidget *audio_label = gtk_label_new("Audio Settings:");
    gtk_widget_set_halign(audio_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(audio_label, "heading");
    gtk_grid_attach(GTK_GRID(grid), audio_label, 0, 1, 2, 1);
    
    GtkWidget *sample_rate_label = gtk_label_new("Sample Rate:");
    gtk_widget_set_halign(sample_rate_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), sample_rate_label, 0, 2, 1, 1);
    
    GtkWidget *sample_rate_info = gtk_label_new("Determined by JACK");
    gtk_widget_set_halign(sample_rate_info, GTK_ALIGN_START);
    gtk_widget_add_css_class(sample_rate_info, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), sample_rate_info, 1, 2, 1, 1);
    
    GtkWidget *buffer_size_label = gtk_label_new("Buffer Size:");
    gtk_widget_set_halign(buffer_size_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), buffer_size_label, 0, 3, 1, 1);
    
    GtkWidget *buffer_size_info = gtk_label_new("Determined by JACK");
    gtk_widget_set_halign(buffer_size_info, GTK_ALIGN_START);
    gtk_widget_add_css_class(buffer_size_info, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), buffer_size_info, 1, 3, 1, 1);
    
    // Add grid to content area
    gtk_box_append(GTK_BOX(content_area), grid);
    
    // Connect response signal
    g_signal_connect(dialog, "response", G_CALLBACK(on_settings_response), data);
    /* Ensure data is freed after the dialog has been destroyed to avoid
     * freeing objects that GTK widgets may still reference during teardown.
     */
    g_signal_connect(dialog, "destroy", G_CALLBACK(on_settings_destroy), data);
    
    // Free themes array
    for (i = 0; i < theme_count; i++) {
        g_free(themes[i]);
    }
    g_free(themes);
    
    // Show dialog
    gtk_window_present(GTK_WINDOW(dialog));
}

void ariel_set_default_theme () {
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
    
}