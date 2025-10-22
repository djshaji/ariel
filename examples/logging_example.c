// Example usage of the Ariel logging system
#include "ariel.h"

void example_function(int some_int, const char* some_string) {
    // Using the macro with your requested template format
    ariel_log(INFO, "Processing %d items with name %s", some_int, some_string);
    ariel_log(WARN, "Low memory warning: %d%% used", 85);
    ariel_log(ERROR, "Failed to load file: %s", some_string);
    
    // Using the convenience macros (recommended)
    ARIEL_INFO("Application started successfully");
    ARIEL_WARN("Configuration file not found, using defaults");
    ARIEL_ERROR("Database connection failed after %d attempts", 3);
    
    // Demonstrate log level filtering
    ariel_log_set_level(ARIEL_LOG_WARN); // Only show WARN and ERROR
    ARIEL_INFO("This will not be shown");
    ARIEL_WARN("This will be shown");
    ARIEL_ERROR("This will also be shown");
    
    // Reset to show all logs
    ariel_log_set_level(ARIEL_LOG_INFO);
}

/*
Example output:
[14:23:45] INFO  example.c:7 example_function() - Processing 42 items with name test.nam
[14:23:45] WARN  example.c:8 example_function() - Low memory warning: 85% used
[14:23:45] ERROR example.c:9 example_function() - Failed to load file: test.nam
[14:23:45] INFO  example.c:12 example_function() - Application started successfully
[14:23:45] WARN  example.c:13 example_function() - Configuration file not found, using defaults
[14:23:45] ERROR example.c:14 example_function() - Database connection failed after 3 attempts
[14:23:45] WARN  example.c:18 example_function() - This will be shown
[14:23:45] ERROR example.c:19 example_function() - This will also be shown
*/