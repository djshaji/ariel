# LV2 Features in Ariel Plugin Host

## Overview
Ariel implements essential LV2 extension features to provide comprehensive plugin compatibility and professional host capabilities.

## Implemented Features

### 1. LV2 URID Map/Unmap (Core Feature)
- **URI**: `http://lv2plug.in/ns/ext/urid#map` / `#unmap`
- **Purpose**: Efficient URI to integer mapping for real-time communication
- **Implementation**: Hash table-based mapping with automatic ID assignment
- **Usage**: All plugins receive consistent URI mappings across sessions

### 2. LV2 Options (Essential Feature)
- **URI**: `http://lv2plug.in/ns/ext/options#options`
- **Purpose**: Host option discovery and configuration
- **Implementation**: Basic options array to satisfy plugin requirements
- **Benefits**: Prevents "Options feature missing" plugin failures

### 3. LV2 State Support (Persistence Feature)
- **URI**: `http://lv2plug.in/ns/ext/state#makePath`
- **Purpose**: Plugin state file management and directory creation
- **Implementation**: Automatic state directory creation in `~/.config/ariel/plugin_state/`
- **Integration**: Works with existing preset system

### 4. LV2 Atom Path Support (NEW)
- **URI**: `http://lv2plug.in/ns/ext/state#mapPath`
- **Purpose**: File path mapping and resource management
- **Implementation**: 
  - `ariel_map_absolute_path()`: Maps absolute paths to plugin-accessible locations
  - `ariel_map_abstract_path()`: Converts abstract paths to absolute paths
  - Automatic file copying to state directory
- **Benefits**: 
  - Safe file access for plugins
  - Resource isolation and management
  - Support for plugins that work with external files (samples, models, etc.)

## Usage Examples

### URID Mapping
```c
// Plugin requests URI mapping
LV2_URID atom_path_urid = map->map(map->handle, LV2_ATOM__Path);
LV2_URID patch_set_urid = map->map(map->handle, LV2_PATCH__Set);
```

### Path Mapping
```c
// Plugin maps external file to safe location
char *safe_path = map_path->absolute_path(handle, "/external/sample.wav");
// Result: ~/.config/ariel/plugin_state/sample.wav

// Plugin accesses state file
char *full_path = map_path->abstract_path(handle, "plugin_data.bin");
// Result: ~/.config/ariel/plugin_state/plugin_data.bin
```

### State Directory Structure
```
~/.config/ariel/
├── plugin_state/          # LV2 State files and resources
│   ├── sample_files/      # Mapped audio samples
│   ├── neural_models/     # AI model files
│   └── plugin_data/       # Plugin-specific state data
├── presets/               # Individual plugin presets
└── chain_presets/         # Plugin chain presets
```

## Plugin Compatibility

### Enhanced Support For:
- **Neural Amp Modeler**: Model file loading via Path mapping
- **Ratatouille**: Neural model and impulse response file management
- **Sample-based plugins**: Audio file mapping and loading
- **DPF plugins**: Full Options and URID support
- **Modern LV2 plugins**: Comprehensive feature set

### Feature Requirements Met:
- ✅ URID Map/Unmap (Required by most modern plugins)
- ✅ Options (Required by DPF framework plugins)
- ✅ State Make Path (Required for plugins with external state)
- ✅ State Map Path (Required for plugins with file resources)
- ✅ Atom support (Required for advanced messaging)

## Technical Implementation

### URID Map Performance
- Hash table lookup: O(1) average case
- Memory efficient: Only active URIs stored
- Thread-safe: Atomic operations for ID generation

### Path Security
- Sandboxed file access within state directory
- Automatic file copying for external resources
- Safe filename generation to prevent path traversal

### Memory Management
- Proper feature cleanup on plugin removal
- Reference counting for shared resources
- Leak-free URID map implementation

## Future Enhancements

### Planned Features:
- **LV2 Worker**: Background thread processing
- **LV2 Log**: Plugin logging integration
- **LV2 Time**: Transport position information
- **LV2 Midi**: MIDI event handling

### Advanced Path Features:
- Resource versioning and caching
- Automatic file format conversion
- Shared resource management across plugins
- Network resource downloading and caching

## Testing

### Verification Steps:
1. Plugin loads without "feature missing" errors
2. URID mappings appear in console output
3. State directory created automatically
4. Path mapping functions execute successfully
5. Resource files copied to safe locations

### Example Test Output:
```
URID Map: http://lv2plug.in/ns/ext/atom#Path -> 1
URID Map: http://lv2plug.in/ns/ext/atom#String -> 2  
Created LV2 features: URID Map/Unmap, Options, State Make Path, Map Path
Mapped absolute path: /external/file.wav -> ~/.config/ariel/plugin_state/file.wav
```

This comprehensive LV2 feature implementation makes Ariel compatible with the vast majority of modern LV2 plugins and provides a solid foundation for advanced plugin hosting capabilities.