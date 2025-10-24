# Building Ariel for Windows

This document describes how to cross-compile Ariel for Windows from a Linux system using MinGW-w64.

## Prerequisites

### Install Cross-Compilation Toolchain

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install gcc-mingw-w64-x86-64 mingw-w64-tools wine64
```

#### Fedora/RHEL/CentOS  
```bash
sudo dnf install mingw64-gcc mingw64-gtk4 mingw64-lilv wine
```

#### Arch Linux
```bash
sudo pacman -S mingw-w64-gcc mingw-w64-gtk4 wine
```

### Install Windows Dependencies

You'll need Windows versions of the required libraries. Some package managers provide these:

#### Fedora (with mingw64 packages)
```bash
sudo dnf install mingw64-gtk4-devel mingw64-lilv-devel mingw64-jack-audio-connection-kit
```

For other distributions, you may need to build these dependencies from source or find pre-compiled mingw64 versions.

## Cross-Compilation Process

### 1. Automated Build (Recommended)

The project includes an automated build script:

```bash
git clone https://github.com/djshaji/ariel.git
cd ariel
./build-windows.sh
```

This script will:
- Check for required cross-compiler tools
- Configure the build for Windows 
- Compile the executable
- Output `build-windows/ariel.exe`

### 2. Manual Cross-Compilation

If you prefer to build manually:

```bash
# Configure build for Windows cross-compilation
meson setup build-windows --cross-file cross/windows-x86_64.txt

# Compile
meson compile -C build-windows

# Optional: Strip debug symbols to reduce size
x86_64-w64-mingw32-strip build-windows/ariel.exe
```

### 3. Create Portable Package

Use the packaging script to bundle all required DLLs:

```bash
./package-windows.sh
```

This creates a `ariel-windows-x86_64/` directory containing:
- `ariel.exe` - The main executable
- All required DLL dependencies (GTK4, lilv, MinGW runtime)
- Application data files (themes, CSS)
- Windows-specific documentation

## Cross-File Configuration

The build uses `cross/windows-x86_64.txt` which defines:

```ini
[binaries]
c = 'x86_64-w64-mingw32-gcc'
cpp = 'x86_64-w64-mingw32-g++'
ar = 'x86_64-w64-mingw32-ar'
strip = 'x86_64-w64-mingw32-strip'
pkgconfig = 'x86_64-w64-mingw32-pkg-config'
exe_wrapper = 'wine64'

[built-in options]
c_args = ['-DWIN32', '-D_WIN32_WINNT=0x0601']
c_link_args = ['-static-libgcc']

[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
```

## Testing the Windows Build

### Using Wine (Linux)

You can test the Windows executable on Linux using Wine:

```bash
# Install Wine if not already installed
sudo apt install wine64  # Ubuntu/Debian
sudo dnf install wine    # Fedora
sudo pacman -S wine      # Arch

# Run the Windows executable
wine64 build-windows/ariel.exe
```

### On Windows

Transfer the `ariel-windows-x86_64/` directory to a Windows machine and:

1. Install JACK for Windows from https://jackaudio.org/downloads/
2. Run `ariel.exe` or `ariel.bat`

## Troubleshooting

### Common Issues

#### 1. Missing Cross-Compiler
```
Error: x86_64-w64-mingw32-gcc not found!
```
**Solution**: Install the MinGW-w64 toolchain for your distribution.

#### 2. Missing Windows Libraries
```
Dependency "gtk4" not found
```
**Solution**: Install mingw64 development packages or build dependencies from source.

#### 3. PKG_CONFIG_PATH Issues
```
Package 'lilv-0' not found
```
**Solution**: Set PKG_CONFIG_PATH to include mingw64 libraries:
```bash
export PKG_CONFIG_PATH="/usr/x86_64-w64-mingw32/lib/pkgconfig:$PKG_CONFIG_PATH"
```

#### 4. Wine Testing Issues
```
wine: cannot find L"C:\\windows\\system32\\winemenubuilder.exe"
```
**Solution**: This is normal Wine output and doesn't affect testing.

### Dependency Resolution

If automatic dependency detection fails, you can manually specify library paths:

```bash
meson setup build-windows --cross-file cross/windows-x86_64.txt \
  -Dpkg_config_path="/usr/x86_64-w64-mingw32/lib/pkgconfig"
```

## Windows Runtime Requirements

The final Windows executable requires:

### Essential DLLs (included in package)
- MinGW runtime: `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`
- GTK4 stack: `libgtk-4-1.dll`, `libglib-2.0-0.dll`, etc.
- lilv libraries: `liblilv-0.dll`, `libserd-0.dll`, `libsord-0.dll`

### External Dependencies (user must install)
- **JACK for Windows**: Audio backend (https://jackaudio.org/)
- **LV2 Plugins**: Audio effect plugins (optional)
- **Windows 7+**: Target OS requirement

## Performance Notes

- The Windows build targets Windows 7+ (`_WIN32_WINNT=0x0601`)
- Uses static libgcc linking to reduce DLL dependencies
- Executable is built with Windows subsystem for proper GUI behavior
- Debug symbols are stripped by default for smaller distribution size

## Distribution

To create a distributable package:

```bash
# Create package
./package-windows.sh

# Create ZIP archive
zip -r ariel-windows-x86_64.zip ariel-windows-x86_64/

# Upload or distribute the ZIP file
```

The resulting ZIP file is completely portable and can be extracted anywhere on a Windows system.