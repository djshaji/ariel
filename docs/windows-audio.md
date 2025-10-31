# Windows Audio Configuration for Ariel - IMPLEMENTED ✅

## Overview
Ariel now supports native Windows audio using WASAPI (Windows Audio Session API), providing low-latency audio performance without requiring external audio servers like JACK.

## Architecture

### Audio Backend Selection
- **Linux/Unix**: Uses JACK Audio Connection Kit
- **Windows**: Uses WASAPI (Windows Audio Session API)
- Automatic backend selection at compile time using `#ifdef _WIN32`

### WASAPI Implementation Features
- **Low-latency audio streaming** in shared mode with minimum buffer periods
- **Automatic device detection** for both input and output devices
- **Real-time audio processing thread** with `THREAD_PRIORITY_TIME_CRITICAL`
- **Stereo audio support** with automatic mono plugin handling
- **COM-based device enumeration** using `IMMDeviceEnumerator`
- **Event-driven audio processing** using Windows event objects
- **Proper resource cleanup** with comprehensive error handling

## Components

### Core Files
- `src/audio/wasapi_client.c` - WASAPI implementation
- `src/audio/engine.c` - Modified for Windows/Linux backend selection
- `src/ui/settings.c` - Enhanced with Windows device selection UI

### Key Functions
- `ariel_wasapi_start()` - Initialize and start WASAPI audio engine
- `ariel_wasapi_stop()` - Stop and cleanup WASAPI resources
- `ariel_wasapi_enumerate_devices()` - List available audio devices
- `ariel_wasapi_audio_thread()` - Real-time audio processing thread

## Settings Dialog Integration ✅

### Windows-Specific UI Elements
When running on Windows, the settings dialog now includes:

1. **Input Device Selection**
   - Dropdown listing all available WASAPI input devices
   - Automatic device enumeration using `eCapture` data flow
   - UTF-8 device name conversion for proper display

2. **Output Device Selection**
   - Dropdown listing all available WASAPI output devices  
   - Automatic device enumeration using `eRender` data flow
   - UTF-8 device name conversion for proper display

3. **Audio Settings Section**
   - Sample rate display: "Determined by WASAPI" (instead of JACK)
   - Buffer size display: "Determined by WASAPI" (instead of JACK)

## Technical Details

### Audio Processing Flow
1. **Device Initialization**: Gets default or selected audio endpoints
2. **Audio Client Setup**: Configures `IAudioClient` with minimum latency periods
3. **Service Client Creation**: Obtains `IAudioRenderClient` and `IAudioCaptureClient`
4. **Thread Creation**: Starts real-time audio processing thread
5. **Event-Driven Processing**: Uses Windows event objects for audio callbacks
6. **Plugin Chain Processing**: Integrates with existing LV2 plugin architecture

### Memory Management
- Separate input/output audio buffers for stereo processing
- Proper COM object lifecycle management with reference counting
- Event handle cleanup and thread synchronization
- Buffer allocation sized to maximum of input/output buffer requirements

### Error Handling
- Comprehensive HRESULT checking for all WASAPI calls
- Graceful fallback when input devices are not available
- Detailed error logging using Ariel's logging system
- Safe resource cleanup on initialization failures

## Build System Integration ✅

### Meson Configuration
- Automatic WASAPI source inclusion for Windows builds
- Windows audio library linking: `-lole32 -luuid -lwinmm -lksuser`
- GUID definition support with `#define COBJMACROS`
- Cross-compilation compatibility with MinGW-w64

### Library Dependencies
- **ole32**: COM object creation and management
- **uuid**: GUID definitions for WASAPI interfaces  
- **winmm**: Windows multimedia functions
- **ksuser**: Kernel streaming user mode library

## Usage

### Linux/Unix
```bash
# Uses JACK - no changes needed
./ariel
```

### Windows
```bash
# Uses WASAPI automatically
ariel.exe
```

### Settings Configuration
1. Launch Ariel on Windows
2. Open Settings dialog (gear icon in header bar)
3. Select desired input/output devices from dropdowns
4. Audio engine will use selected devices on next start

## Implementation Status ✅

- ✅ WASAPI audio engine implementation
- ✅ Device enumeration and selection UI
- ✅ Real-time audio processing thread
- ✅ Integration with existing LV2 plugin architecture  
- ✅ Automatic Windows/Linux backend selection
- ✅ Build system integration with MinGW-w64
- ✅ Resource management and error handling
- ✅ Cross-platform compatibility maintained

## Future Enhancements

- Device selection persistence in configuration
- Exclusive mode support for even lower latency
- ASIO driver support for professional audio interfaces
- Sample rate and buffer size configuration options
- Device change notification handling