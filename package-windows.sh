#!/bin/bash

# Windows packaging script for Ariel
# Creates a portable Windows distribution with all required DLLs

set -e

BUILDDIR="build-windows"
PACKAGEDIR="ariel-windows-x86_64"
MINGW_PREFIX="/usr/x86_64-w64-mingw32"

echo "Creating Windows package for Ariel..."

# Check if Windows build exists
if [ ! -f "$BUILDDIR/ariel.exe" ]; then
    echo "Windows build not found. Building first..."
    ./build-windows.sh
fi

# Create package directory
if [ -d "$PACKAGEDIR" ]; then
    echo "Removing existing package directory..."
    rm -rf "$PACKAGEDIR"
fi

mkdir -p "$PACKAGEDIR"

echo "Copying executable..."
cp "$BUILDDIR/ariel.exe" "$PACKAGEDIR/"

echo "Copying required DLLs..."

# Copy project-specific Windows libraries first
echo "Copying project lilv libraries..."
if [ -d "win32" ]; then
    cp win32/*.dll "$PACKAGEDIR/"
    echo "  Copied lilv and dependencies from win32/"
fi

# Function to copy DLL and its dependencies
copy_dll_deps() {
    local dll_path="$1"
    local dll_name=$(basename "$dll_path")
    
    if [ -f "$dll_path" ] && [ ! -f "$PACKAGEDIR/$dll_name" ]; then
        echo "  Copying $dll_name"
        cp "$dll_path" "$PACKAGEDIR/"
        
        # Find dependencies using objdump
        local deps=$(x86_64-w64-mingw32-objdump -p "$dll_path" 2>/dev/null | grep "DLL Name:" | awk '{print $3}' | grep -v "^KERNEL32.dll\|^USER32.dll\|^ADVAPI32.dll\|^msvcrt.dll\|^GDI32.dll" || true)
        
        for dep in $deps; do
            local dep_path=$(find "$MINGW_PREFIX" -name "$dep" 2>/dev/null | head -1)
            if [ -n "$dep_path" ]; then
                copy_dll_deps "$dep_path"
            fi
        done
    fi
}

# Copy essential Windows DLLs
echo "Copying MinGW runtime..."
copy_dll_deps "$MINGW_PREFIX/bin/libgcc_s_seh-1.dll"
copy_dll_deps "$MINGW_PREFIX/bin/libwinpthread-1.dll"
copy_dll_deps "$MINGW_PREFIX/bin/libstdc++-6.dll"

# Copy GTK4 and dependencies
echo "Copying GTK4 libraries..."
for lib in libgtk-4-1.dll libglib-2.0-0.dll libgobject-2.0-0.dll libgio-2.0-0.dll libgdk_pixbuf-2.0-0.dll libcairo-2.dll libpango-1.0-0.dll libpangocairo-1.0-0.dll; do
    dll_path=$(find "$MINGW_PREFIX" -name "$lib" 2>/dev/null | head -1)
    if [ -n "$dll_path" ]; then
        copy_dll_deps "$dll_path"
    fi
done

# Copy lilv and dependencies
echo "Copying lilv libraries..."
for lib in liblilv-0.dll libserd-0.dll libsord-0.dll; do
    dll_path=$(find "$MINGW_PREFIX" -name "$lib" 2>/dev/null | head -1)
    if [ -n "$dll_path" ]; then
        copy_dll_deps "$dll_path"
    fi
done

# Copy JACK if available
echo "Copying JACK libraries..."
jack_dll=$(find "$MINGW_PREFIX" -name "libjack*.dll" 2>/dev/null | head -1)
if [ -n "$jack_dll" ]; then
    copy_dll_deps "$jack_dll"
else
    echo "  JACK DLL not found - user will need to install JACK for Windows"
fi

# Copy themes and data
echo "Copying application data..."
mkdir -p "$PACKAGEDIR/share/ariel"
if [ -f "data/ariel-theme.css" ]; then
    cp "data/ariel-theme.css" "$PACKAGEDIR/share/ariel/"
fi

if [ -d "themes" ]; then
    cp -r "themes" "$PACKAGEDIR/share/ariel/"
fi

# Create README for Windows users
cat > "$PACKAGEDIR/README-Windows.txt" << 'EOF'
Ariel for Windows
================

This is a portable distribution of Ariel LV2 Host for Windows.

Installation:
1. Extract this folder anywhere on your computer
2. Install JACK Audio Connection Kit for Windows from:
   https://jackaudio.org/downloads/
3. Run ariel.exe

Requirements:
- Windows 7 or later (64-bit)
- JACK Audio Connection Kit
- LV2 plugins (optional, for audio effects)

The application includes all necessary GTK4 and lilv libraries.

For more information, visit: https://github.com/djshaji/ariel
EOF

# Create a simple launcher batch file
cat > "$PACKAGEDIR/ariel.bat" << 'EOF'
@echo off
cd /d "%~dp0"
start "" ariel.exe
EOF

echo "✅ Windows package created: $PACKAGEDIR/"
echo "Package contents:"
ls -la "$PACKAGEDIR/"

echo ""
echo "To distribute:"
echo "  zip -r ariel-windows-x86_64.zip $PACKAGEDIR/"
echo ""
echo "Package size: $(du -sh "$PACKAGEDIR" | cut -f1)"