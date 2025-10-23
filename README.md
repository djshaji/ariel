# <img width="475" height="419" alt="logo-sm" src="https://github.com/user-attachments/assets/6e2529ad-4590-4fc1-8f8e-a93cd7784ebc" />
A cross-platform LV2 host built with GTK4 and lilv.
## Features
- **Real-time LV2 plugin hosting** with lilv integration
- **Intelligent plugin search** - Quickly find plugins among 865+ discovered LV2 plugins by name, author, or URI
- **Category filtering** - Browse plugins by type (Distortion, Reverb, Analyzer, etc.) with 40+ categories
- **Professional theme collection** - 20+ built-in dark themes including Dracula, Nord, Gruvbox, One Dark, Solarized, and more
- **Custom CSS theming** - Load custom styles from `~/.config/ariel/style.css` for personalized appearance
- **Settings dialog** - Easy theme switching and configuration management
- **Drag and drop support** for intuitive plugin loading 
- **Parameter controls** with real-time adjustment for all plugin parameters
- **File parameter support** - Load audio files, neural models, and impulse responses directly into plugins
- **Multi-file parameter plugins** - Full support for plugins with multiple file parameters (like Ratatouille)
- **Enhanced file dialogs** - Support for .wav, .nam, .ir, .json, and other audio/model formats
- **Preset management** - Save and load individual plugin presets
- **Chain presets** - Save entire plugin chains with all parameters
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

1. **Plugin Discovery**
   - View all available LV2 plugins in the left panel
   - Use the search box to quickly find plugins by name, author, or URI
   - Filter plugins by category using the dropdown (Distortion, Reverb, Analyzer, etc.)
   - Search is case-insensitive and searches across plugin names, authors, categories, and URIs
   - Over 40 plugin categories automatically detected from LV2 metadata
   - Click on a plugin to load it into the active chain

2. **Loading Plugins**
   - Browse the plugin list on the left side of the window
   - Drag plugins from the list to the active plugins area on the right
   - Or double-click a plugin to load it

3. **Plugin Parameters**
   - Control knobs and sliders appear automatically for each loaded plugin
   - **File Parameters**: Click "Choose File..." buttons to load audio files, neural models, or impulse responses
   - Supported formats: .wav, .nam, .nammodel, .ir, .json, .aidadspmodel, .aidiax, .cabsim
   - Multi-file plugins (like Ratatouille) show separate controls for each file parameter
   - Adjust parameters in real-time while audio is playing
   - Parameters are saved with your session

4. **Plugin Management**
   - Remove individual plugins with the "Remove" button
   - Use "Remove All" to clear all active plugins
   - Plugins process audio in the order they appear in the list

5. **Theme Selection**
   - Access the Settings dialog from the header bar menu
   - Choose from 20+ professional dark themes:
     - **Popular themes**: Dracula, Nord, Gruvbox, One Dark, Solarized Dark
     - **Classic themes**: Tomorrow Night, Zenburn, Arc, Material Darker
     - **Specialized themes**: Atomic, Ayu Dark, Badwolf, Blackboard, Cobalt2
     - **Custom themes**: Midnight, Charcoal, Slate, Spacegray, and more
   - Themes feature sophisticated styling with gradients, animations, and accessibility support
   - Settings are automatically saved and restored on application restart

6. **Preset Management**
   - **Individual Plugin Presets**: Save/load presets for individual plugins using the "Save" and "Load" buttons on each plugin
   - **Chain Presets**: Save entire plugin chains with all parameters using "Save Chain" and "Load Chain" buttons
   - Presets are stored in `~/.config/ariel/presets/` (individual) and `~/.config/ariel/chain_presets/` (chains)
   - Chain presets preserve plugin order, all parameter values, and bypass states

7. **Transport Controls**
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
- **Neural Amp Modelers**: Neural Amp Modeler plugin with .nam/.nammodel file support
- **Multi-parameter Plugins**: Ratatouille (4 file parameters), and other complex plugins
- **IR Processors**: Impulse response plugins with .wav/.ir file loading
- **Generators**: Synthesizers, oscillators, noise generators
- **Analyzers**: Spectrum analyzers, meters, tuners
- **MIDI Effects**: Note processors, arpegiators (MIDI support coming soon)

## Customization

### Built-in Theme Collection

Ariel includes 20+ professionally designed dark themes accessible through the Settings dialog:

#### Popular Developer Themes
- **Dracula** - Purple-tinted dark theme popular in coding
- **Nord** - Arctic-inspired blue theme
- **Gruvbox Dark** - Retro groove color scheme
- **One Dark** - Atom editor's default dark theme
- **Solarized Dark** - Scientifically designed color palette
- **Tomorrow Night** - Clean, readable dark theme

#### Editor & IDE Themes  
- **Arc** - Flat dark theme with blue accents
- **Ayu Dark** - Rust editor theme
- **Badwolf** - Vim colorscheme adaptation
- **Blackboard** - TextMate editor theme
- **Cobalt2** - Wes Bos' coding theme
- **Darcula** - IntelliJ IDEA dark theme
- **Material Darker** - Google Material Design dark
- **Zenburn** - Low-contrast retro theme

#### Specialized Themes
- **Atomic** - Red-accented scientific theme
- **Midnight** - Deep blue midnight colors
- **Charcoal** - Neutral gray professional theme
- **Slate** - Modern slate gray theme
- **Spacegray** - Minimalist space-inspired theme

All themes feature:
- **Modern GTK4 styling** with gradients and animations
- **Accessibility support** with high contrast options
- **Consistent design language** across all UI elements
- **Professional appearance** suitable for audio production

### Custom CSS Theming

Ariel supports custom CSS styling for complete visual customization:

1. **Create CSS file**: `~/.config/ariel/style.css`
2. **Add custom styles**: Use standard CSS syntax to modify the interface
3. **Available selectors**: `window`, `.title-2`, `.suggested-action`, `.card`, `button`, etc.
4. **Example styles**:
   ```css
   /* Custom Ariel theme */
   window {
       background: linear-gradient(45deg, #1a1a2e, #16213e);
   }
   
   .title-2 {
       color: #00d4aa;
       font-weight: bold;
   }
   
   .suggested-action {
       background: linear-gradient(45deg, #00d4aa, #0f3460);
   }
   ```

### Tips

- **Theme Selection**: Use the Settings dialog to instantly switch between 20+ professional themes
- **Mono Plugins**: Automatically work with stereo audio - mono output is duplicated to both channels
- **Plugin Order**: Drag plugins in the active list to reorder them
- **Performance**: Start with smaller buffer sizes in JACK for lower latency
- **Plugin Discovery**: Ariel caches plugin information for faster startup
- **Custom Styling**: Create `~/.config/ariel/style.css` for complete visual customization
- **Theme Persistence**: Selected themes are automatically saved and restored

Instructions for running the application and using its features will be provided here.
## Contributing
Contributions are welcome! Please see the CONTRIBUTING.md file for guidelines.
## License
This project is licensed under the MIT License. See the LICENSE file for details.
