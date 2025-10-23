# Packages

This document provides instructions for building and distributing Ariel LV2 Host packages across different Linux distributions and package formats.

## Prerequisites

Before building packages, ensure you have the development dependencies installed:

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential meson ninja-build pkg-config \
                 libgtk-4-dev liblilv-dev libjack-jackd2-dev \
                 git devscripts debhelper dh-make
```

### Fedora/RHEL/CentOS
```bash
sudo dnf install gcc gcc-c++ meson ninja-build pkgconfig \
                 gtk4-devel lilv-devel jack-audio-connection-kit-devel \
                 git rpm-build rpmdevtools
```

### Arch Linux
```bash
sudo pacman -S base-devel meson ninja pkgconf gtk4 lilv jack2 git
```

## Building from Source

### Standard Build
```bash
git clone https://github.com/djshaji/ariel.git
cd ariel
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

### Custom Installation Prefix
```bash
meson setup builddir --prefix=/usr/local
meson compile -C builddir
sudo meson install -C builddir
```

## Package Formats

### 1. DEB Package (Debian/Ubuntu)

#### Manual DEB Creation
```bash
# Create package directory structure
mkdir -p ariel-deb/DEBIAN
mkdir -p ariel-deb/usr/bin
mkdir -p ariel-deb/usr/share/applications
mkdir -p ariel-deb/usr/share/icons/hicolor/scalable/apps
mkdir -p ariel-deb/usr/share/ariel/themes

# Build and install to package directory
meson setup builddir --prefix=/usr
meson compile -C builddir
DESTDIR=$PWD/ariel-deb meson install -C builddir

# Create control file
cat > ariel-deb/DEBIAN/control << EOF
Package: ariel-lv2-host
Version: 1.0.0
Section: sound
Priority: optional
Architecture: amd64
Depends: libgtk-4-1, liblilv-0-0, libjack-jackd2-0
Maintainer: Your Name <your.email@example.com>
Description: Ariel LV2 Plugin Host
 A modern cross-platform LV2 plugin host built with GTK4 and lilv
 for real-time audio processing with JACK audio backend support.
EOF

# Build DEB package
dpkg-deb --build ariel-deb ariel-lv2-host_1.0.0_amd64.deb
```

#### Using debuild
```bash
# Create debian packaging files
dh_make --single --native --packagename ariel-lv2-host

# Edit debian/control, debian/rules, etc.
# Then build:
debuild -us -uc
```

### 2. RPM Package (Fedora/RHEL/CentOS)

#### Create RPM Spec File
```bash
# Create RPM build environment
rpmdev-setuptree

# Create spec file
cat > ~/rpmbuild/SPECS/ariel.spec << 'EOF'
Name:           ariel-lv2-host
Version:        1.0.0
Release:        1%{?dist}
Summary:        Modern LV2 plugin host with GTK4 interface

License:        GPLv3+
URL:            https://github.com/djshaji/ariel
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc gcc-c++ meson ninja-build pkgconfig
BuildRequires:  gtk4-devel lilv-devel jack-audio-connection-kit-devel
Requires:       gtk4 lilv jack-audio-connection-kit

%description
Ariel is a cross-platform LV2 plugin host built with GTK4 and lilv
for real-time audio processing. The application provides a modern
interface for loading and managing LV2 plugins with JACK audio backend support.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%license LICENSE
%doc README.md
%{_bindir}/ariel
%{_datadir}/applications/ariel.desktop
%{_datadir}/icons/hicolor/*/apps/ariel.*
%{_datadir}/ariel/

%changelog
* $(date +"%%a %%b %%d %%Y") Your Name <your.email@example.com> - 1.0.0-1
- Initial package
EOF

# Create source tarball
tar czf ~/rpmbuild/SOURCES/ariel-lv2-host-1.0.0.tar.gz ariel/

# Build RPM
rpmbuild -ba ~/rpmbuild/SPECS/ariel.spec
```

### 3. AppImage (Universal Linux)

#### Create AppImage
```bash
# Download AppImage tools
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage

# Create AppDir structure
mkdir -p Ariel.AppDir/usr

# Build and install to AppDir
meson setup builddir --prefix=/usr
meson compile -C builddir
DESTDIR=$PWD/Ariel.AppDir meson install -C builddir

# Create AppRun script
cat > Ariel.AppDir/AppRun << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/ariel" "$@"
EOF
chmod +x Ariel.AppDir/AppRun

# Copy desktop file and icon
cp Ariel.AppDir/usr/share/applications/ariel.desktop Ariel.AppDir/
cp Ariel.AppDir/usr/share/icons/hicolor/scalable/apps/ariel.svg Ariel.AppDir/

# Create AppImage
./appimagetool-x86_64.AppImage Ariel.AppDir Ariel-x86_64.AppImage
```

### 4. Flatpak Package

#### Create Flatpak Manifest
```bash
# Create flatpak directory
mkdir flatpak
cd flatpak

cat > org.example.Ariel.yml << 'EOF'
app-id: org.example.Ariel
runtime: org.freedesktop.Platform
runtime-version: '23.08'
sdk: org.freedesktop.Sdk
command: ariel

finish-args:
  - --socket=x11
  - --socket=wayland
  - --socket=pulseaudio
  - --device=all
  - --filesystem=home
  - --share=network

modules:
  - name: lilv
    buildsystem: meson
    sources:
      - type: archive
        url: https://download.drobilla.net/lilv-0.24.20.tar.xz
        sha256: 4fb082b9b8136d0d8d1a4b1b6e2676b68f24e7d84e7e38177a6479905eb62fb8

  - name: ariel
    buildsystem: meson
    sources:
      - type: dir
        path: ../
EOF

# Build Flatpak
flatpak-builder build-dir org.example.Ariel.yml --force-clean
flatpak build-export repo build-dir
flatpak build-bundle repo ariel.flatpak org.example.Ariel
```

### 5. Snap Package

#### Create snapcraft.yaml
```bash
mkdir snap
cat > snap/snapcraft.yaml << 'EOF'
name: ariel-lv2-host
version: '1.0.0'
summary: Modern LV2 plugin host
description: |
  Ariel is a cross-platform LV2 plugin host built with GTK4 and lilv
  for real-time audio processing with JACK audio backend support.

grade: stable
confinement: strict
base: core22

apps:
  ariel-lv2-host:
    command: usr/bin/ariel
    desktop: usr/share/applications/ariel.desktop
    plugs:
      - home
      - audio-playback
      - audio-record
      - desktop
      - desktop-legacy
      - wayland
      - x11

parts:
  ariel:
    plugin: meson
    source: .
    build-packages:
      - libgtk-4-dev
      - liblilv-dev
      - libjack-jackd2-dev
      - pkg-config
    stage-packages:
      - libgtk-4-1
      - liblilv-0-0
      - libjack-jackd2-0
EOF

# Build snap
snapcraft
```

## Distribution-Specific Instructions

### Arch Linux (AUR)

Create a PKGBUILD file for the Arch User Repository:

```bash
cat > PKGBUILD << 'EOF'
pkgname=ariel-lv2-host
pkgver=1.0.0
pkgrel=1
pkgdesc="Modern LV2 plugin host with GTK4 interface"
arch=('x86_64')
url="https://github.com/djshaji/ariel"
license=('GPL3')
depends=('gtk4' 'lilv' 'jack2')
makedepends=('meson' 'ninja')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/ariel-$pkgver"
    arch-meson . build
    meson compile -C build
}

package() {
    cd "$srcdir/ariel-$pkgver"
    meson install -C build --destdir "$pkgdir"
}
EOF

# Build package
makepkg -si
```

### Gentoo Linux

Create an ebuild file:

```bash
cat > ariel-lv2-host-1.0.0.ebuild << 'EOF'
EAPI=8

inherit meson

DESCRIPTION="Modern LV2 plugin host with GTK4 interface"
HOMEPAGE="https://github.com/djshaji/ariel"
SRC_URI="https://github.com/djshaji/ariel/archive/v${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"

DEPEND="
    gui-libs/gtk:4
    media-libs/lilv
    media-sound/jack2
"
RDEPEND="${DEPEND}"
BDEPEND="
    dev-build/meson
    dev-build/ninja
    virtual/pkgconfig
"

S="${WORKDIR}/ariel-${PV}"
EOF
```

## Continuous Integration

### GitHub Actions for Package Building

Create `.github/workflows/packages.yml`:

```yaml
name: Build Packages

on:
  push:
    tags:
      - 'v*'
  pull_request:
    branches: [ main ]

jobs:
  build-deb:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install build-essential meson ninja-build pkg-config \
                           libgtk-4-dev liblilv-dev libjack-jackd2-dev
      - name: Build DEB package
        run: |
          # Add DEB build steps here
          
  build-rpm:
    runs-on: fedora-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo dnf install gcc gcc-c++ meson ninja-build pkgconfig \
                           gtk4-devel lilv-devel jack-audio-connection-kit-devel
      - name: Build RPM package
        run: |
          # Add RPM build steps here

  build-appimage:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build AppImage
        run: |
          # Add AppImage build steps here
```

## Installation Instructions

### From DEB Package
```bash
sudo dpkg -i ariel-lv2-host_1.0.0_amd64.deb
sudo apt-get install -f  # Fix dependencies if needed
```

### From RPM Package
```bash
sudo rpm -ivh ariel-lv2-host-1.0.0-1.fc39.x86_64.rpm
# or
sudo dnf install ariel-lv2-host-1.0.0-1.fc39.x86_64.rpm
```

### From AppImage
```bash
chmod +x Ariel-x86_64.AppImage
./Ariel-x86_64.AppImage
```

### From Flatpak
```bash
flatpak install ariel.flatpak
flatpak run org.example.Ariel
```

### From Snap
```bash
sudo snap install ariel-lv2-host_1.0.0_amd64.snap --dangerous
```

## Notes

- Replace placeholder email addresses and names with actual maintainer information
- Update version numbers and checksums as needed
- Test packages on target distributions before release
- Consider setting up automated package building with CI/CD
- Ensure all dependencies are correctly specified for each package format
- Include proper licensing information in all packages


