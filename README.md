# Ariel
A cross-platform LV2 host built with GTK4 and lilv.
## Features
- **Real-time LV2 plugin hosting** with lilv integration
- **Drag and drop support** for intuitive plugin loading 
- **Parameter controls** with real-time adjustment for all plugin parameters
- **Preset management** - Save and load individual plugin presets
- **Chain presets** - Save and load entire plugin chains with all parameters
- **Plugin bypass** functionality for A/B testing
- **Transport controls** - Play, stop, record with state management
- **Auto-start audio engine** for immediate plugin processing
- **Mono plugin support** with automatic stereo conversion
- **Cross-platform compatibility** - Linux, Windows, and macOS
- **Modern GTK4 interface** with responsive design
## Installation

### Prerequisites

Ariel requires the following dependencies:

- **GTK4** (>= 4.0) - Modern UI toolkit
- **lilv** - LV2 plugin host library  
- **JACK Audio Connection Kit** - Real-time audio server
- **Meson** - Build system
- **C compiler** (GCC or Clang)

### Linux Installation

#### Ubuntu/Debian
```bash
# Install dependencies
sudo apt update
sudo apt install libgtk-4-dev liblilv-dev libjack-jackd2-dev meson build-essential

# Clone the repository
git clone https://github.com/djshaji/ariel.git
cd ariel

# Configure build
meson setup builddir

# Compile
meson compile -C builddir

# Run
./builddir/ariel
```

#### Fedora/RHEL/CentOS
```bash
# Install dependencies
sudo dnf install gtk4-devel lilv-devel jack-audio-connection-kit-devel meson gcc

# Clone and build
git clone https://github.com/djshaji/ariel.git
cd ariel
meson setup builddir
meson compile -C builddir

# Run
./builddir/ariel
```

#### Arch Linux
```bash
# Install dependencies
sudo pacman -S gtk4 lilv jack2 meson gcc

# Clone and build
git clone https://github.com/djshaji/ariel.git
cd ariel
meson setup builddir
meson compile -C builddir

# Run
./builddir/ariel
```

### Audio Setup

Ariel requires JACK to be running for audio processing:

#### Start JACK (if not running)
```bash
# Check if JACK is running
jack_control status

# Start JACK with default settings
jack_control start

# Or use QJackCtl for graphical JACK control
qjackctl
```

#### PipeWire Users
If you're using PipeWire (common on modern Linux), JACK compatibility is usually enabled by default:
```bash
# Check if PipeWire-JACK is active
systemctl --user is-active pipewire-jack
```

### Building from Source

#### Development Build
```bash
# Clone repository
git clone https://github.com/djshaji/ariel.git
cd ariel

# Setup build directory
meson setup builddir

# Compile with debug info
meson compile -C builddir

# Clean rebuild (if needed)
rm -rf builddir
meson setup builddir
meson compile -C builddir
```

#### Release Build
```bash
# Configure for release
meson setup builddir --buildtype=release

# Compile optimized version
meson compile -C builddir
```

### Installation to System

```bash
# Install to system directories (optional)
sudo meson install -C builddir

# Uninstall (if needed)
sudo ninja -C builddir uninstall
```

### Troubleshooting

#### Common Issues

1. **Missing LV2 plugins**: Install LV2 plugin packages
   ```bash
   # Ubuntu/Debian
   sudo apt install lv2-dev calf-plugins eq10q-lv2
   
   # Fedora
   sudo dnf install lv2-devel calf-plugins
   
   # Arch
   sudo pacman -S lv2 calf
   ```

2. **JACK not running**: Start JACK audio server
   ```bash
   jack_control start
   ```

3. **Audio device permissions**: Add user to audio group
   ```bash
   sudo usermod -a -G audio $USER
   # Log out and back in
   ```

4. **GTK4 theme warnings**: Install GTK4 themes
   ```bash
   # Ubuntu/Debian
   sudo apt install adwaita-icon-theme-full
   ```

#### Build Issues

- **Missing dependencies**: Ensure all development packages are installed
- **Meson errors**: Try updating Meson: `pip3 install --user --upgrade meson`
- **Compilation fails**: Check GCC/Clang version (GCC 8+ recommended)

### Performance Tips

- Use a low-latency kernel for better audio performance
- Adjust JACK buffer size for your needs (lower = less latency, higher = more stable)
- Close unnecessary applications when using real-time audio processing

## Usage

### Getting Started

1. **Launch Ariel**
   ```bash
   ./builddir/ariel
   ```
   The audio engine will automatically start when the application launches.

2. **Loading Plugins**
   - Browse the plugin list on the left side of the window
   - Drag plugins from the list to the active plugins area on the right
   - Or double-click a plugin to load it

3. **Plugin Parameters**
   - Control knobs and sliders appear automatically for each loaded plugin
   - Adjust parameters in real-time while audio is playing
   - Parameters are saved with your session

4. **Plugin Management**
   - Remove individual plugins with the "Remove" button
   - Use "Remove All" to clear all active plugins
   - Plugins process audio in the order they appear in the list

5. **Preset Management**
   - **Individual Plugin Presets**: Save/load presets for individual plugins using the "Save" and "Load" buttons on each plugin
   - **Chain Presets**: Save entire plugin chains with all parameters using "Save Chain" and "Load Chain" buttons
   - Presets are stored in `~/.config/ariel/presets/` (individual) and `~/.config/ariel/chain_presets/` (chains)
   - Chain presets preserve plugin order, all parameter values, and bypass states

6. **Transport Controls**
   - Use Play/Stop buttons in the header bar
   - Record button for future recording functionality

### Audio Setup

Before using Ariel, ensure your audio setup is working:

1. **Connect Audio Sources**
   - Connect your audio input (microphone, instrument, etc.) to your audio interface
   - Or use system audio sources

2. **JACK Connections** (if using JACK directly)
   - Use `qjackctl` or `jack_lsp` to see available ports
   - Connect Ariel's input/output ports to your audio interface
   - Example with command line:
     ```bash
     # Connect system input to Ariel
     jack_connect system:capture_1 ariel:input_left
     jack_connect system:capture_2 ariel:input_right
     
     # Connect Ariel output to system
     jack_connect ariel:output_left system:playback_1
     jack_connect ariel:output_right system:playback_2
     ```

### Plugin Types Supported

- **Audio Effects**: Reverb, delay, distortion, EQ, compressors, etc.
- **Generators**: Synthesizers, oscillators, noise generators
- **Analyzers**: Spectrum analyzers, meters, tuners
- **MIDI Effects**: Note processors, arpegiators (MIDI support coming soon)

### Tips

- **Mono Plugins**: Automatically work with stereo audio - mono output is duplicated to both channels
- **Plugin Order**: Drag plugins in the active list to reorder them
- **Performance**: Start with smaller buffer sizes in JACK for lower latency
- **Plugin Discovery**: Ariel caches plugin information for faster startup

Instructions for running the application and using its features will be provided here.
## Contributing
Contributions are welcome! Please see the CONTRIBUTING.md file for guidelines.
## License
This project is licensed under the MIT License. See the LICENSE file for details.