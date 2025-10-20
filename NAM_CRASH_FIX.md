# Neural Amp Modeler Crash Fix

## Problem
Loading NAM files in the Neural Amp Modeler plugin caused a segmentation fault in the `lv2_atom_forge_init` function.

## Root Cause
The crash was caused by incorrect type handling in the URID mapping system. The code was attempting to cast `ArielURIDMap*` directly to `LV2_URID_Map*`, but these are different structures:

- `ArielURIDMap*` is an internal structure that manages URI-to-ID mappings
- `LV2_URID_Map*` is the actual LV2 standard structure that contains function pointers

## Stack Trace
```
#0  0x0000000000479e80 in ??? ()
#1  0x000000000040bb6e in lv2_atom_forge_init (forge=0x7fffffffcba0, map=0x454ff0)
#2  0x000000000040edb7 in ariel_active_plugin_set_file_parameter (plugin=0x869c780, file_path=...)
#3  0x00000000004043f3 in on_file_dialog_open_finish (source=0x8320ab0, result=0x7d392c0, user_data=0x8321 7d0)
```

## Solution
The fix involved properly retrieving the `LV2_URID_Map*` structure from the manager's features array instead of incorrectly casting the `ArielURIDMap*`:

### Before (Incorrect):
```c
if (manager && manager->urid_map) {
    plugin->urid_map = (LV2_URID_Map*)manager->urid_map; // Wrong cast
}
```

### After (Correct):
```c
if (manager && manager->urid_map && manager->features) {
    // Get the actual LV2_URID_Map from the features
    plugin->urid_map = NULL;
    for (int i = 0; manager->features[i]; i++) {
        if (strcmp(manager->features[i]->URI, LV2_URID__map) == 0) {
            plugin->urid_map = (LV2_URID_Map*)manager->features[i]->data;
            break;
        }
    }
    
    if (!plugin->urid_map) {
        g_warning("Could not find LV2_URID_Map in features");
        return NULL;
    }
}
```

## Additional Safety Measures
Added comprehensive validation in `ariel_active_plugin_set_file_parameter`:

1. **Pointer validation**: Check if URID map pointer is in valid memory range
2. **Function pointer validation**: Ensure the map function pointer is not NULL
3. **Buffer validation**: Verify atom buffer size and allocation
4. **URID validation**: Check all required URIDs are properly initialized
5. **Forge operation validation**: Check return values of all atom forge operations

## Test Results
- ✅ Neural Amp Modeler plugin loads successfully
- ✅ File dialog opens and works correctly
- ✅ NAM files can be selected without crash
- ✅ Atom messages are properly formatted and sent to plugin
- ✅ Application exits cleanly

## Debug Output (Success)
```
Selected neural model file: /path/to/model.nam
Sending file parameter to plugin: /path/to/model.nam
Initializing LV2 Atom Forge with URID map: 0x9336490 (map function: 0x409016)
Sent file path to Neural Amp Modeler: /path/to/model.nam (sequence size: 176, forge offset: 168)
Neural model loaded: /path/to/model.nam
```

## Files Modified
- `src/audio/active_plugin.c`: Fixed URID map initialization and added safety checks
- `src/ui/parameter_controls.c`: Added additional validation in file dialog callback

The crash is now completely resolved and the Neural Amp Modeler plugin works correctly with file loading functionality.