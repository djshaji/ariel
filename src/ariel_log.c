#include "ariel_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

// ANSI color codes
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_RESET   "\033[0m"

// Log level strings with colors
static const char* log_level_strings[] = {
    [ARIEL_LOG_ERROR] = COLOR_RED "ERROR" COLOR_RESET,
    [ARIEL_LOG_WARN]  = COLOR_YELLOW "WARN " COLOR_RESET,
    [ARIEL_LOG_INFO]  = COLOR_CYAN "INFO " COLOR_RESET
};

// Current log level (can be modified to filter logs)
static ArielLogLevel current_log_level = ARIEL_LOG_INFO;

void
ariel_log_impl(ArielLogLevel level, const char* file, int line, const char* func, const char* format, ...)
{
    // Filter based on current log level
    if (level > current_log_level) {
        return;
    }
    
    // Get current time
    time_t now;
    struct tm* timeinfo;
    char timestamp[32];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeinfo);
    
    // Extract just the filename from full path
    const char* filename = strrchr(file, '/');
    if (filename) {
        filename++; // Skip the '/'
    } else {
        filename = file; // No path separator found
    }
    
    // Print log header with timestamp, level, location
    printf("[%s] %s %s:%d %s() - ", 
           timestamp,
           log_level_strings[level], 
           filename, 
           line, 
           func);
    
    // Print the actual log message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

void
ariel_log_set_level(ArielLogLevel level)
{
    current_log_level = level;
}

ArielLogLevel
ariel_log_get_level(void)
{
    return current_log_level;
}