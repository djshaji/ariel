# LV2 Log Extension Implementation

## Overview

The LV2 Log extension provides a standardized logging interface that allows LV2 plugins to output debug, warning, and error messages through the host application. This implementation integrates the LV2 log extension with Ariel's existing logging system.

## Implementation Details

### Files Modified

1. **include/ariel.h** - Added LV2 log header include
2. **src/audio/plugin_manager.c** - Implemented log callback functions and feature integration

### Key Components

#### 1. LV2 Log Callback Functions

```c
static int ariel_log_printf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, ...);
static int ariel_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char *fmt, va_list ap);
```

These functions:
- Map LV2 log types (Error, Warning, Note, Trace) to Ariel log levels (ERROR, WARN, INFO)
- Format messages with a `[LV2]` prefix for easy identification
- Route all LV2 plugin logs through Ariel's unified logging system

#### 2. URID Mapping

The implementation pre-maps important LV2 log URIDs:
- `LV2_LOG__Error` → `ERROR` level
- `LV2_LOG__Warning` → `WARN` level  
- `LV2_LOG__Note` → `INFO` level
- `LV2_LOG__Trace` → `INFO` level

#### 3. Feature Integration

The log feature is added to the LV2 features array provided to plugins:

```c
// LV2 Log feature
LV2_Log_Log *log_feature = g_malloc0(sizeof(LV2_Log_Log));
log_feature->handle = manager;
log_feature->printf = ariel_log_printf;
log_feature->vprintf = ariel_log_vprintf;

features[6] = g_malloc0(sizeof(LV2_Feature));
features[6]->URI = LV2_LOG__log;
features[6]->data = log_feature;
```

## Benefits

### For Plugin Developers
- Standardized logging interface across different LV2 hosts
- Rich formatting capabilities with printf-style syntax
- Multiple log levels for different message types
- Host-managed log routing and formatting

### For Users
- Unified log output format with timestamps and source location
- Colored console output for easy visual distinction
- Plugin messages integrated with host application logs
- Consistent log file organization

### For Debugging
- Plugin internal state visibility
- Error reporting and diagnostic information
- Performance profiling data from plugins
- Development and troubleshooting support

## Usage Example

Plugins can use the log extension like this:

```c
// In plugin code
if (plugin_features.log) {
    plugin_features.log->printf(plugin_features.log->handle, 
                               log_Note_URID, 
                               "Processing %d samples at %.1f Hz", 
                               sample_count, sample_rate);
}
```

This would appear in Ariel's log as:
```
[14:15:26] INFO  plugin_manager.c:38 ariel_log_printf() - [LV2] Processing 1024 samples at 48000.0 Hz
```

## Technical Notes

### Log Level Mapping

| LV2 Log Type | Ariel Log Level | Color | Description |
|--------------|-----------------|-------|-------------|
| LV2_LOG__Error | ERROR | Red | Critical errors |
| LV2_LOG__Warning | WARN | Yellow | Warnings and issues |
| LV2_LOG__Note | INFO | Green | Informational messages |
| LV2_LOG__Trace | INFO | Green | Debug traces |

### Memory Management

- Log feature structures are allocated during feature creation
- Proper cleanup in `ariel_free_lv2_features()`
- Thread-safe message formatting with local buffers

### Performance Considerations

- Minimal overhead for log message processing
- Printf-style formatting only when messages are actually logged
- URID lookup caching for efficient log type mapping

## Compatibility

This implementation follows the LV2 Log specification and is compatible with:
- Standard LV2 plugins that use the log extension
- Development and debugging tools
- Other LV2 hosts that implement the log extension

## Integration Testing

The implementation has been tested with:
- Neural Amp Modeler plugin loading and operation
- Message formatting and log level mapping
- Feature availability detection by plugins
- Memory cleanup and resource management

## Future Enhancements

Potential improvements:
- Log filtering by plugin or log level
- Log file rotation and archiving
- Plugin-specific log configuration
- Performance metrics collection through log data