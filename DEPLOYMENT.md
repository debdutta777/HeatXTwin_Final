# Qt Deployment Guide - HeatXTwin Pro

## Problem Solved ✅

**Error**: "Could not find the Qt platform plugin 'windows'"

**Cause**: Qt applications need platform-specific plugins and DLLs to run outside the development environment.

**Solution**: Use `windeployqt` to automatically deploy all required dependencies.

## Deployment Steps

### Automatic Deployment (Recommended)

```powershell
# Navigate to the executable directory
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release"

# Run windeployqt
C:\Users\DEBDUTTA\vcpkg\installed\x64-windows\tools\qt6\bin\windeployqt.exe --release --no-translations HeatXTwin_Pro.exe
```

### What Gets Deployed

**Qt Core DLLs** (in same directory as .exe):
- ✅ `Qt6Core.dll` - Core functionality
- ✅ `Qt6Gui.dll` - GUI framework
- ✅ `Qt6Widgets.dll` - Widget library
- ✅ `Qt6Charts.dll` - Chart components
- ✅ `Qt6Network.dll` - Network support
- ✅ `Qt6OpenGL.dll` - OpenGL rendering
- ✅ `Qt6OpenGLWidgets.dll` - OpenGL widgets

**Platform Plugins** (`platforms/` subdirectory):
- ✅ `qwindows.dll` - **Critical** Windows platform plugin

**Image Format Plugins** (`imageformats/` subdirectory):
- ✅ `qgif.dll` - GIF support
- ✅ `qico.dll` - ICO support
- ✅ `qjpeg.dll` - JPEG support

**Style Plugins** (`styles/` subdirectory):
- ✅ `qmodernwindowsstyle.dll` - Modern Windows styling

**Other Plugins**:
- `generic/` - Generic plugins (touch input)
- `networkinformation/` - Network info
- `tls/` - TLS/SSL support

**Dependencies**:
- ✅ `icudt74.dll`, `icuin74.dll`, `icuuc74.dll` - Internationalization
- ✅ `freetype.dll`, `harfbuzz.dll` - Font rendering
- ✅ `libpng16.dll` - PNG image support
- ✅ `zlib1.dll`, `zstd.dll`, `bz2.dll` - Compression
- ✅ `pcre2-16.dll` - Regular expressions
- ✅ `double-conversion.dll` - Number conversion
- ✅ `D3Dcompiler_47.dll` - DirectX shader compiler

## Verifying Deployment

Check that the platform plugin exists:

```powershell
Test-Path "platforms\qwindows.dll"
# Should return: True
```

Check DLL count:

```powershell
(Get-ChildItem *.dll).Count
# Should return: ~25 DLLs
```

## Running the Application

### From Deployed Directory

```powershell
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release"
.\HeatXTwin_Pro.exe
```

### From Any Location

```powershell
Start-Process "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release\HeatXTwin_Pro.exe"
```

### From Windows Explorer

Simply double-click `HeatXTwin_Pro.exe` in:
```
C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release\
```

## Creating Portable Distribution

To create a distributable version:

```powershell
# 1. Create distribution folder
New-Item -ItemType Directory -Path "C:\HeatXTwin_Distribution" -Force

# 2. Copy entire Release directory
Copy-Item "build\bin\Release\*" -Destination "C:\HeatXTwin_Distribution\" -Recurse

# 3. Copy config file
Copy-Item "configs\baseline.toml" -Destination "C:\HeatXTwin_Distribution\configs\"

# 4. Create a README
"HeatXTwin Pro 2.0 - Simply run HeatXTwin_Pro.exe" | Out-File "C:\HeatXTwin_Distribution\README.txt"

# 5. Zip it
Compress-Archive -Path "C:\HeatXTwin_Distribution\*" -DestinationPath "HeatXTwin_Pro_v2.0.zip"
```

Now you can share `HeatXTwin_Pro_v2.0.zip` with others!

## Troubleshooting

### Still Getting Platform Plugin Error

**Check environment variable**:
```powershell
$env:QT_PLUGIN_PATH
# Should be empty or point to correct location
```

**Manually set plugin path** (if needed):
```powershell
$env:QT_PLUGIN_PATH = "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release"
.\HeatXTwin_Pro.exe
```

### Missing DLL Errors

**Re-run deployment**:
```powershell
Remove-Item platforms, styles, imageformats, generic, networkinformation, tls -Recurse -Force -ErrorAction SilentlyContinue
C:\Users\DEBDUTTA\vcpkg\installed\x64-windows\tools\qt6\bin\windeployqt.exe --release --no-translations HeatXTwin_Pro.exe
```

### Application Crashes on Startup

**Check for conflicting Qt installations**:
```powershell
# Ensure no other Qt paths in system PATH
$env:PATH -split ';' | Where-Object { $_ -like '*qt*' }
```

**Run with debugging**:
```powershell
$env:QT_DEBUG_PLUGINS = 1
.\HeatXTwin_Pro.exe
# This will show detailed plugin loading info
```

## Rebuilding and Redeploying

After code changes:

```powershell
# 1. Rebuild
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build"
cmake --build . --config Release

# 2. Redeploy (updates only changed files)
cd bin\Release
C:\Users\DEBDUTTA\vcpkg\installed\x64-windows\tools\qt6\bin\windeployqt.exe --release --no-translations HeatXTwin_Pro.exe

# 3. Run
.\HeatXTwin_Pro.exe
```

## windeployqt Options

**Full deployment** (includes everything):
```powershell
windeployqt.exe --release HeatXTwin_Pro.exe
```

**Minimal deployment** (no translations, no compiler):
```powershell
windeployqt.exe --release --no-translations --no-compiler-runtime HeatXTwin_Pro.exe
```

**With OpenSSL** (if using secure connections):
```powershell
windeployqt.exe --release --no-translations --force-openssl HeatXTwin_Pro.exe
```

**Debug build deployment**:
```powershell
windeployqt.exe --debug HeatXTwin_Pro.exe
```

## File Size Summary

**Deployed application size**:
- Executable: ~100 KB
- Qt DLLs: ~30 MB
- ICU Data: ~30 MB
- Plugins: ~3 MB
- Other dependencies: ~5 MB
- **Total**: ~70 MB

## CMake Integration (Optional)

To automatically deploy after build, add to `CMakeLists.txt`:

```cmake
# Find windeployqt
find_program(WINDEPLOYQT_EXECUTABLE 
    NAMES windeployqt
    PATHS "${CMAKE_PREFIX_PATH}/tools/qt6/bin"
)

if(WINDEPLOYQT_EXECUTABLE)
    # Custom target to run windeployqt
    add_custom_command(TARGET HeatXTwin_Pro POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE} 
            --release 
            --no-translations
            "$<TARGET_FILE:HeatXTwin_Pro>"
        COMMENT "Running windeployqt..."
    )
endif()
```

Then deployment happens automatically on every build!

## Success Indicators

Application is properly deployed when:

✅ `HeatXTwin_Pro.exe` runs without errors  
✅ Window opens with UI visible  
✅ No console errors about missing plugins  
✅ Charts display correctly  
✅ All buttons and controls work  

## Quick Reference Card

```
┌─────────────────────────────────────────────┐
│  HeatXTwin Pro - Qt Deployment Checklist    │
├─────────────────────────────────────────────┤
│  1. Build:                                  │
│     cmake --build . --config Release        │
│                                             │
│  2. Navigate:                               │
│     cd build\bin\Release                    │
│                                             │
│  3. Deploy:                                 │
│     windeployqt --release HeatXTwin_Pro.exe │
│                                             │
│  4. Run:                                    │
│     .\HeatXTwin_Pro.exe                     │
│                                             │
│  ✅ Done!                                   │
└─────────────────────────────────────────────┘
```

---

**Status**: ✅ Deployment Complete  
**Platform Plugin**: ✅ Installed (`platforms\qwindows.dll`)  
**Application**: ✅ Ready to Run  
**Distribution**: ✅ Ready to Share  

The application should now launch without any Qt platform plugin errors!
