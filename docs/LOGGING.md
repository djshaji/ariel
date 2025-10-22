# Ariel Logging System

A printf-style logger with colored output, timestamps, and source location information.

## Features
- ✅ **Colored Output**: ERROR (red), WARN (yellow), INFO (cyan)
- ✅ **Source Location**: Automatically captures file, line number, and function name
- ✅ **Timestamps**: Shows when each log was generated
- ✅ **Log Level Filtering**: Can be set to show only certain log levels
- ✅ **Printf-style Format**: Supports all standard printf format specifiers

## Usage

### Basic Usage (your requested template)
```c
#include "ariel.h"

// Using your requested template format:
int some_int = 42;
const char* some_char = "test.nam";
ariel_log(WARN, "%d %s", some_int, some_char);

// Other examples:
ariel_log(ERROR, "Failed to load file: %s", filename);
ariel_log(INFO, "Processing %d samples at %d Hz", samples, samplerate);
```

### Convenience Macros (recommended)
```c
ARIEL_ERROR("Database connection failed after %d attempts", 3);
ARIEL_WARN("Configuration file not found, using defaults");
ARIEL_INFO("Application started successfully");
```

### Log Level Control
```c
// Set minimum log level (only WARN and ERROR will be shown)
ariel_log_set_level(ARIEL_LOG_WARN);

// Get current log level
ArielLogLevel level = ariel_log_get_level();

// Reset to show all logs
ariel_log_set_level(ARIEL_LOG_INFO);
```

## Example Output

```
[13:33:01] INFO  main.c:45 main() - Application started successfully
[13:33:01] WARN  config.c:123 load_config() - Configuration file not found, using defaults
[13:33:02] ERROR plugin.c:67 load_plugin() - Failed to load file: missing.so
[13:33:02] INFO  parameter_controls.c:21 on_parameter_changed() - Parameter 2 changed to 0.750
```

## Implementation

The logging system consists of:
- `src/ariel_log.c` - Implementation with color codes and formatting
- `include/ariel_log.h` - Header with macros and function declarations
- Automatically included in `include/ariel.h`

## Color Scheme
- 🔴 **ERROR**: Red text for critical errors
- 🟡 **WARN**: Yellow text for warnings  
- 🔵 **INFO**: Cyan text for informational messages

## Integration

The logging system is already integrated into the Ariel codebase and replaces previous `g_print` calls in critical sections like:
- File parameter error handling
- Plugin parameter changes
- File dialog operations
- Audio engine operations

The system is thread-safe and suitable for real-time audio applications.