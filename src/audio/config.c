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
    // Windows: Use APPDATA with additional error checking
    config_dir = g_get_user_config_dir();
    if (config_dir && strlen(config_dir) > 0) {
        ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
        ARIEL_INFO("Windows config dir: %s", ariel_config_dir ? ariel_config_dir : "NULL");
    } else {
        ARIEL_WARN("Failed to get Windows user config directory");
    }
#elif defined(__APPLE__)
    // macOS: Use ~/Library/Application Support
    const char *home = g_get_home_dir();
    if (home && strlen(home) > 0) {
        ariel_config_dir = g_build_filename(home, "Library", "Application Support", "ariel", NULL);
    } else {
        ARIEL_WARN("Failed to get macOS home directory");
    }
#else
    // Linux/Unix: Use XDG_CONFIG_HOME or ~/.config
    config_dir = g_get_user_config_dir();
    if (config_dir && strlen(config_dir) > 0) {
        ariel_config_dir = g_build_filename(config_dir, "ariel", NULL);
    } else {
        ARIEL_WARN("Failed to get Unix user config directory");
    }
#endif
    
    if (!ariel_config_dir) {
        // Fallback to home directory with additional safety checks
        const char *home = g_get_home_dir();
        if (home && strlen(home) > 0) {
            ariel_config_dir = g_build_filename(home, ".ariel", NULL);
            ARIEL_INFO("Using fallback config dir: %s", ariel_config_dir);
        } else {
            ARIEL_ERROR("Failed to determine any suitable config directory");
            // Final fallback to current directory
            char *current_dir = g_get_current_dir();
            if (current_dir) {
                ariel_config_dir = g_build_filename(current_dir, ".ariel", NULL);
                ARIEL_WARN("Using current directory fallback: %s", ariel_config_dir);
                g_free(current_dir);
            }
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
    if (!config) {
        ARIEL_ERROR("Failed to allocate memory for ArielConfig");
        return NULL;
    }
    
    // Get platform-specific config directory with detailed error reporting
    config->config_dir = get_platform_config_dir();
    if (!config->config_dir) {
        ARIEL_ERROR("Failed to determine config directory - cannot proceed");
        g_free(config);
        return NULL;
    }
    
    // Validate config directory path
    if (strlen(config->config_dir) == 0) {
        ARIEL_ERROR("Config directory path is empty");
        g_free(config->config_dir);
        g_free(config);
        return NULL;
    }
    
    // Ensure config directory exists with detailed error reporting
    if (!ensure_directory_exists(config->config_dir)) {
        ARIEL_ERROR("Failed to create or access config directory: %s", config->config_dir);
        g_free(config->config_dir);
        g_free(config);
        return NULL;
    }
    
    // Set cache file path with validation
    config->cache_file = g_build_filename(config->config_dir, "plugin_cache.ini", NULL);
    if (!config->cache_file) {
        ARIEL_ERROR("Failed to build cache file path");
        g_free(config->config_dir);
        g_free(config);
        return NULL;
    }
    
    ARIEL_INFO("Using config directory: %s", config->config_dir);
    ARIEL_INFO("Plugin cache file: %s", config->cache_file);
    
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