# Windows Console Output Configuration

## Overview
This configuration enables full console output for the Ariel LV2 Host on Windows, making debugging and error reporting much easier.

## Changes Made

### 1. Cross-Compilation Configuration (`cross/windows-x86_64.txt`)
```ini
[built-in options]
c_args = ['-DWIN32', '-D_WIN32_WINNT=0x0601', '-DCONSOLE_ENABLED', '-I../win32/include']
c_link_args = ['-static-libgcc', '-mconsole', '-L../win32/lib']
```

**Key additions:**
- `-DCONSOLE_ENABLED`: Compile-time flag for console support
- `-mconsole`: Links as console application instead of GUI-only

### 2. Enhanced Console Allocation (`src/main.c`)

**Multi-Strategy Console Setup:**
1. **AllocConsole()**: Create new console window
2. **AttachConsole(ATTACH_PARENT_PROCESS)**: Use parent terminal
3. **GetConsoleWindow()**: Detect existing console
4. **OutputDebugStringA()**: Fallback to debug output

**Stream Redirection:**
```c
freopen_s(&fp_out, "CONOUT$", "w", stdout);
freopen_s(&fp_err, "CONOUT$", "w", stderr);
freopen_s(&fp_in, "CONIN$", "r", stdin);
```

**Console Properties:**
- Custom title: "Ariel LV2 Host - Debug Console"
- Proper cleanup on exit
- User prompt to keep console visible

### 3. Testing Tools

#### Console Test Program (`console_test.c`)
Simple test to verify console output functionality:
```bash
x86_64-w64-mingw32-gcc -o console_test.exe console_test.c -mconsole -static-libgcc
wine64 console_test.exe
```

#### Windows Batch Script (`ariel-console.bat`)
For native Windows testing:
```batch
ariel.exe.exe
pause
```

#### Build Script (`build-windows-console.sh`)
Automated build with console support:
```bash
./build-windows-console.sh
```

## Usage Scenarios

### 1. **Wine Testing** (Linux)
```bash
# Test console functionality
wine64 console_test.exe

# Run Ariel with console output
wine64 build-windows-console/ariel.exe.exe
```

### 2. **Windows Command Prompt**
```cmd
# Run from command prompt to see output
ariel.exe.exe

# Or use batch script
ariel-console.bat
```

### 3. **Windows PowerShell**
```powershell
# PowerShell will show console output
.\ariel.exe.exe

# Keep window open
.\ariel.exe.exe; Read-Host "Press Enter to continue"
```

### 4. **Debug Output** (DebugView)
If console allocation fails, output goes to debug stream:
- Download Microsoft DebugView
- Enable "Capture Win32" output
- See debug messages from `OutputDebugStringA()`

## Output Examples

### Successful Console Output
```
Starting Ariel on Windows platform
New console allocated
Windows debug console configured successfully
Console output test: printf working
Console error test: stderr working
COM initialized successfully
Validating environment before app creation
Creating ArielApp instance
...
```

### Console Allocation Failure
```
Starting Ariel on Windows platform
Ariel: Console allocation failed, using debug output
(Messages appear in DebugView or debugger)
```

## Debugging Benefits

### 1. **Crash Analysis**
- See exact crash location and error messages
- Stack traces visible in console
- Memory allocation failures reported

### 2. **Initialization Tracking**
- Step-by-step component loading
- LV2 plugin discovery progress  
- JACK audio system status

### 3. **Performance Monitoring**
- Plugin loading times
- Audio processing statistics
- Memory usage information

### 4. **User Feedback**
- Clear error messages for users
- Installation problem diagnosis
- Configuration validation

## Build Verification

After building, verify console support:

```bash
# 1. Test basic console functionality
wine64 console_test.exe

# 2. Check console window appears
wine64 build-windows-console/ariel.exe.exe

# 3. Verify output appears in terminal
wine64 build-windows-console/ariel.exe.exe 2>&1 | tee ariel-output.log
```

## Troubleshooting

### Console Not Appearing
1. Verify `-mconsole` flag is in link options
2. Check Wine console support: `winecfg` → Graphics → Allow DirectDraw apps to grab mouse
3. Try running from Windows command prompt instead of double-clicking

### No Output Visible  
1. Check if `freopen_s` succeeded
2. Verify `fflush()` calls are present
3. Use DebugView to see `OutputDebugStringA()` messages

### Wine-Specific Issues
1. Update Wine to latest version
2. Install Windows console fonts: `winetricks corefonts`
3. Set Wine to Windows 10 mode: `winecfg`

## Status
✅ **Console output fully enabled for Windows builds**  
✅ **Multiple fallback strategies for console allocation**  
✅ **Debug output available even if console fails**  
✅ **Testing tools provided for verification**  
✅ **Compatible with Wine and native Windows**