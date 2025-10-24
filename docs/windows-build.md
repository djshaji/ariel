# Windows Build Configuration

This document describes the Windows build configuration for Ariel LV2 Host after reorganizing the win32 directory structure.

## Directory Structure

The win32 directory now follows a standard Unix-like layout:

```
win32/
├── bin/           # Windows DLL files
├── etc/           # Configuration files
├── include/       # Header files
│   ├── jack/      # JACK Audio headers
│   ├── lilv-0/    # LV2 lilv headers
│   ├── lv2/       # LV2 specification headers
│   ├── serd-0/    # RDF serialization headers
│   ├── sord-0/    # RDF storage headers
│   ├── sratom-0/  # LV2 atom headers
│   └── zix-0/     # Utility library headers
├── lib/           # Static and import libraries
│   └── pkgconfig/ # pkg-config files for cross-compilation
└── share/         # Shared data files
```

## Build Configuration Changes

### meson.build Updates

1. **Updated lilv dependency detection** for Windows:
   - Uses reorganized include directories from `win32/include/`
   - Links against libraries from `win32/lib/`
   - Includes all necessary LV2 ecosystem headers

2. **Enhanced JACK dependency detection**:
   - Checks for JACK libraries in `win32/lib/`
   - Uses `libjack64.dll.a` import library
   - Includes JACK headers from `win32/include/jack/`

3. **Added theme installation**:
   - Installs complete theme collection for Windows
   - Handles both CSS themes and theme manager script

4. **Windows-specific installation**:
   - Automatically installs DLLs from `win32/bin/` if present
   - Handles shared data from `win32/share/` if available

### Cross-compilation Configuration

Updated `cross/windows-x86_64.txt`:
- Added include path for `win32/include`
- Added library path for `win32/lib`
- Maintains MinGW-w64 compatibility

### Build Script Improvements

Updated `build-windows.sh`:
- Validates new directory structure
- Checks for essential libraries in `win32/lib/`
- Sets appropriate environment variables for cross-compilation
- Uses pkg-config files from `win32/lib/pkgconfig/`

### Packaging Script Updates

Updated `package-windows.sh`:
- Looks for DLLs in `win32/bin/` directory
- Falls back to legacy structure if needed
- Handles both new and old win32 layouts

## Usage

### Building for Windows

```bash
# Ensure win32 directory is properly populated
./build-windows.sh

# Create Windows package
./package-windows.sh
```

### Required Libraries in win32/

Essential libraries that should be present:

**In win32/lib/:**
- `liblilv-0.dll.a` - LV2 plugin host library
- `libserd-0.dll.a` - RDF serialization library
- `libsord-0.dll.a` - RDF storage library
- `libsratom-0.dll.a` - LV2 atom library
- `libzix-0.dll.a` - Utility library
- `libjack64.dll.a` - JACK Audio (optional)

**In win32/bin/ (for runtime):**
- `liblilv-0.dll`
- `libserd-0.dll`
- `libsord-0.dll`
- `libsratom-0.dll`
- `libzix-0.dll`
- `jack64.dll` (optional)

**In win32/include/:**
- Complete header trees for all dependencies
- LV2 specification headers
- JACK Audio headers (optional)

## Compatibility

This configuration maintains backward compatibility with existing Windows builds while providing better organization and easier dependency management. The build system automatically detects the directory structure and adapts accordingly.

## Troubleshooting

1. **Missing libraries**: Ensure all required `.dll.a` files are in `win32/lib/`
2. **Header not found**: Check that include directories are properly populated
3. **pkg-config errors**: Verify `win32/lib/pkgconfig/` contains necessary `.pc` files
4. **Runtime DLL errors**: Ensure runtime DLLs are in `win32/bin/` for packaging

The reorganized structure makes it easier to maintain Windows dependencies and provides better separation between build-time and runtime requirements.