#ifndef ARIEL_LOG_H
#define ARIEL_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

// Log levels
typedef enum {
    ARIEL_LOG_ERROR = 0,
    ARIEL_LOG_WARN  = 1,
    ARIEL_LOG_INFO  = 2
} ArielLogLevel;

// Internal logging function (don't call directly)
void ariel_log_impl(ArielLogLevel level, const char* file, int line, const char* func, const char* format, ...);

// Set/get current log level for filtering
void ariel_log_set_level(ArielLogLevel level);
ArielLogLevel ariel_log_get_level(void);

// Convenience macros that automatically capture file, line, and function
#define ariel_log(level, format, ...) \
    ariel_log_impl(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// Convenience macros for each log level  
#define ARIEL_ERROR(format, ...) ariel_log(ARIEL_LOG_ERROR, format, ##__VA_ARGS__)
#define ARIEL_WARN(format, ...)  ariel_log(ARIEL_LOG_WARN, format, ##__VA_ARGS__)
#define ARIEL_INFO(format, ...)  ariel_log(ARIEL_LOG_INFO, format, ##__VA_ARGS__)

// Legacy compatibility - matches your requested template
#define ERROR ARIEL_LOG_ERROR
#define WARN  ARIEL_LOG_WARN
#define INFO  ARIEL_LOG_INFO

#ifdef __cplusplus
}
#endif

#endif // ARIEL_LOG_H