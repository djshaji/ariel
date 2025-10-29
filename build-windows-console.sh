#!/bin/bash

# Build script for Windows with console output enabled

set -e  # Exit on any error

echo "=== Building Ariel for Windows with Console Output ==="

# Check if cross-compiler is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "ERROR: x86_64-w64-mingw32-gcc not found!"
    echo "Please install MinGW-w64 cross-compiler:"
    echo "  Ubuntu/Debian: sudo apt install gcc-mingw-w64-x86-64"
    echo "  Fedora: sudo dnf install mingw64-gcc"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

echo "Found MinGW-w64 cross-compiler"

# Clean previous build
if [ -d "build-windows-console" ]; then
    echo "Cleaning previous build..."
    rm -rf build-windows-console
fi

# Build console test first
echo "Building console test..."
x86_64-w64-mingw32-gcc -o console_test.exe console_test.c -mconsole -static-libgcc
if [ $? -eq 0 ]; then
    echo "Console test built successfully: console_test.exe"
else
    echo "ERROR: Console test build failed"
    exit 1
fi

# Configure meson build with console support
echo "Configuring Windows build with console support..."
meson setup build-windows-console --cross-file cross/windows-x86_64.txt --buildtype=debug

# Build main application
echo "Building Ariel..."
meson compile -C build-windows-console

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "Built files:"
    echo "  - console_test.exe (Simple console test)"
    echo "  - build-windows-console/ariel.exe.exe (Main application)"
    echo ""
    echo "Testing with Wine:"
    echo "To test console output:"
    echo "  wine64 console_test.exe"
    echo "  wine64 build-windows-console/ariel.exe.exe"
    echo ""
    echo "Or use the batch script on Windows:"
    echo "  ariel-console.bat"
else
    echo "ERROR: Build failed"
    exit 1
fi