#include "ariel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static char *
get_platform_config_dir(void)
{
    const char *config_dir = NULL;
    char *ariel_config_dir = NULL;
    
#ifdef G_OS_WIN32
    // Windows: Use APPDATA
    config_dir = g_get_user_config_dir();
    if (config_dir) {
        ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
    }
#elif defined(__APPLE__)
    // macOS: Use ~/Library/Application Support
    const char *home = g_get_home_dir();
    if (home) {
        ariel_config_dir = g_build_filename(home, "Library", "Application Support", "ariel", NULL);
    }
#else
    // Linux/Unix: Use XDG_CONFIG_HOME or ~/.config
    config_dir = g_get_user_config_dir();
    if (config_dir) {
        ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
    }
#endif
    
    if (!ariel_config_dir) {
        // Fallback to home directory
        const char *home = g_get_home_dir();
        if (home) {
            ariel_config_dir = g_build_filename(home, ".ariel", NULL);
        }
    }
    
    return ariel_config_dir;
}

static gboolean
ensure_directory_exists(const char *path)
{
    if (!path) return FALSE;
    
    // Check if directory already exists
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        return TRUE;
    }
    
    // Try to create the directory
    if (g_mkdir_with_parents(path, 0755) == 0) {
        g_print("Created config directory: %s\n", path);
        return TRUE;
    }
    
    g_warning("Failed to create config directory %s: %s", path, g_strerror(errno));
    return FALSE;
}

ArielConfig *
ariel_config_new(void)
{
    ArielConfig *config = g_malloc0(sizeof(ArielConfig));
    
    // Get platform-specific config directory
    config->config_dir = get_platform_config_dir();
    if (!config->config_dir) {
        g_warning("Failed to determine config directory");
        g_free(config);
        return NULL;
    }
    
    // Ensure config directory exists
    if (!ensure_directory_exists(config->config_dir)) {
        g_free(config->config_dir);
        g_free(config);
        return NULL;
    }
    
    // Set cache file path
    config->cache_file = g_build_filename(config->config_dir, "plugin_cache.ini", NULL);
    
    g_print("Using config directory: %s\n", config->config_dir);
    g_print("Plugin cache file: %s\n", config->cache_file);
    
    return config;
}

void
ariel_config_free(ArielConfig *config)
{
    if (!config) return;
    
    g_free(config->config_dir);
    g_free(config->cache_file);
    g_free(config);
}

const char *
ariel_config_get_dir(ArielConfig *config)
{
    return config ? config->config_dir : NULL;
}

const char *
ariel_config_get_cache_file(ArielConfig *config)
{
    return config ? config->cache_file : NULL;
}