#!/bin/bash

# Build script for Windows cross-compilation
# Requires x86_64-w64-mingw32-gcc toolchain

set -e

echo "Building Ariel for Windows using MinGW-w64..."

# Check if cross-compiler is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: x86_64-w64-mingw32-gcc not found!"
    echo "Please install MinGW-w64 cross-compiler:"
    echo "  Ubuntu/Debian: sudo apt install gcc-mingw-w64-x86-64"
    echo "  Fedora: sudo dnf install mingw64-gcc"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

# Check if Windows libraries are available
if [ ! -d "win32" ] || [ ! -d "win32/lib" ] || [ ! -d "win32/include" ]; then
    echo "Error: Windows libraries not found in win32/ directory!"
    echo "Please ensure the win32 directory contains lib/ and include/ subdirectories"
    exit 1
fi

# Check for essential libraries
ESSENTIAL_LIBS=("liblilv-0.dll.a" "libserd-0.dll.a" "libsord-0.dll.a" "libsratom-0.dll.a" "libzix-0.dll.a")
for lib in "${ESSENTIAL_LIBS[@]}"; do
    if [ ! -f "win32/lib/$lib" ]; then
        echo "Warning: Essential library $lib not found in win32/lib/"
    fi
done

echo "Found Windows libraries in win32/ directory structure"
echo "Available include directories:"
ls -la win32/include/ | head -10
echo "Available libraries:"
ls -la win32/lib/lib*.dll.a | head -10

# Create build directory for Windows
BUILDDIR="build-windows"
if [ -d "$BUILDDIR" ]; then
    echo "Cleaning existing Windows build directory..."
    rm -rf "$BUILDDIR"
fi

echo "Setting up Windows build directory..."
# Add our win32/lib/pkgconfig directory to PKG_CONFIG_PATH, along with mingw system path
MINGW_PKG_CONFIG_PATH="/usr/x86_64-w64-mingw32/sys-root/mingw/lib/pkgconfig"
WIN32_PKG_CONFIG_PATH="$(pwd)/win32/lib/pkgconfig"
export PKG_CONFIG_PATH="$WIN32_PKG_CONFIG_PATH:$MINGW_PKG_CONFIG_PATH"
echo "PKG_CONFIG_PATH: $PKG_CONFIG_PATH"

# Set additional environment variables for the win32 directory
export CFLAGS="-I$(pwd)/win32/include $CFLAGS"
export LDFLAGS="-L$(pwd)/win32/lib $LDFLAGS"

meson setup "$BUILDDIR" --cross-file cross/windows-x86_64.txt

echo "Building for Windows..."
meson compile -C "$BUILDDIR"

if [ $? -eq 0 ]; then
    echo "✅ Windows build successful!"
    echo "Executable: $BUILDDIR/ariel.exe"
    
    # Show file info
    file "$BUILDDIR/ariel.exe"
    
    # Optional: Strip symbols to reduce size
    if command -v x86_64-w64-mingw32-strip &> /dev/null; then
        echo "Stripping debug symbols..."
        x86_64-w64-mingw32-strip "$BUILDDIR/ariel.exe"
        echo "Final executable size: $(du -h "$BUILDDIR/ariel.exe" | cut -f1)"
    fi
else
    echo "❌ Windows build failed!"
    exit 1
fi