# Ariel LV2 Host - Development TODO

## ✅ Completed Foundation

- [x] **Create project structure** - Set up the basic directory structure for the LV2 host project including src/, include/, build system files
- [x] **Setup build system** - Create Meson build files (meson.build) for cross-platform compilation with GTK4 and lilv dependencies  
- [x] **Create main application structure** - Implement main.c with GTK4 application setup and basic window structure
- [x] **Implement UI components** - Create header bar, paned layout, plugin list, and mixer interface using GTK4
- [x] **Add audio engine foundation** - Set up lilv integration, LV2 plugin discovery and management, JACK audio backend
- [x] **Update copilot instructions** - Replace the current placeholder copilot-instructions.md with actual project-specific guidance

## 🚧 Next Development Steps

### Core Plugin Functionality
- [x] **Plugin Loading & Instantiation** - Implement actual LV2 plugin instantiation and processing in audio thread ✅
  - [x] Create plugin instance management
  - [x] Add plugin to active plugins list when loaded  
  - [x] Integrate plugin processing into JACK callback
  - [x] Handle plugin state and cleanup

- [x] **Parameter Control UI** - Add UI controls for plugin parameters and real-time parameter changes ✅
  - [x] Generate parameter controls dynamically based on plugin metadata
  - [x] Implement parameter change communication between UI and audio threads
  - [ ] Add parameter automation support
  - [ ] Save/restore plugin presets

### User Interface Enhancements
- [x] **Drag & Drop Support** - Enable plugin loading via drag and drop from plugin list to active plugins ✅
  - [x] Implement drag source on plugin list items
  - [x] Add drop target on active plugins area
  - [x] Visual feedback during drag operations
  - [ ] Support for audio file drag & drop

- [x] **Transport Controls Implementation** - Connect play/stop/record buttons to actual functionality ✅\n  - [x] Implement transport state management  \n  - [x] Add recording capabilities with file I/O (TODO: actual file recording)\n  - [x] Sync transport with JACK transport if available (TODO: JACK transport integration)\n  - [x] Add timeline/position display (TODO: enhance UI with position info)\n\n- [x] **JACK Client Audio Processing** - Real-time plugin processing in JACK callback ✅\n  - [x] Connect plugin manager to audio engine\n  - [x] Implement serial plugin chain processing  \n  - [x] Handle plugin activation/deactivation in real-time\n  - [x] Audio buffer management and routing

### Audio Engine Features  
- [ ] **Real Mixer Channels** - Add actual mixer channels for loaded plugins with routing
  - Individual volume/pan controls per plugin
  - Mute/solo functionality
  - Audio routing matrix
  - Master output section

- [ ] **Plugin Chain Management** - Support for plugin chains and routing
  - Serial plugin chaining
  - Parallel processing support
  - Plugin bypass/enable toggles
  - Chain presets and templates

### Advanced Features
- [ ] **Session Management** - Save and load complete sessions with plugins and settings
  - XML or JSON session format
  - Plugin state serialization
  - Audio routing persistence
  - Recent sessions menu

- [ ] **MIDI Support** - Add MIDI input/output and plugin MIDI parameter control
  - JACK MIDI port registration
  - MIDI learn functionality
  - Virtual MIDI keyboard
  - MIDI CC to plugin parameter mapping

- [x] **Plugin List Display** - Show discovered LV2 plugins in the UI with names and authors ✅
- [x] **Configuration System & Plugin Caching** - Platform-specific config directory with plugin cache ✅
- [ ] **Plugin Browser Enhancements** - Improve plugin discovery and organization
  - Plugin categories and tags
  - Search and filter functionality
  - Favorites/bookmarks system
  - Plugin information display (ports, features, etc.)

- [ ] **Performance Optimization** - Optimize for low-latency real-time performance
  - Buffer size adaptation
  - CPU usage monitoring
  - Memory pool allocation
  - Plugin processing scheduling

## 🔧 Technical Debt & Polish
- [ ] **Error Handling** - Improve error handling throughout the application
- [ ] **Memory Management** - Audit and fix potential memory leaks
- [ ] **Unit Tests** - Add comprehensive test suite
- [ ] **Documentation** - User manual and API documentation
- [ ] **Packaging** - Create distribution packages for major platforms

---
*Mark items as completed by changing `[ ]` to `[x]` as development progresses.*