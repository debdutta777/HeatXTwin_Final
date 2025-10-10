# HeatXTwin Pro - Setup and Installation Guide

## 📋 Prerequisites Installation

### 1. Install Qt 6

**Option A: Qt Online Installer (Recommended)**
1. Download from: https://www.qt.io/download-qt-installer
2. Run the installer
3. Select Qt 6.5+ (or latest)
4. **Important**: Select these components:
   - Qt Charts
   - MSVC 2019 64-bit (or MSVC 2022 64-bit)
   - CMake and Ninja
5. Note the installation path (e.g., `C:\Qt\6.5.3\msvc2019_64`)

**Option B: vcpkg**
```powershell
# Clone vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install Qt6
.\vcpkg install qt6-base:x64-windows
.\vcpkg install qt6-charts:x64-windows
```

### 2. Install Visual Studio Build Tools

If not already installed:
1. Download: https://visualstudio.microsoft.com/downloads/
2. Install "Build Tools for Visual Studio 2022"
3. Select "Desktop development with C++"

### 3. Install CMake

If not already installed:
1. Download from: https://cmake.org/download/
2. Add to PATH during installation

## 🔧 Configuration

### Setting Qt6 Path for CMake

**Method 1: Environment Variable (Persistent)**
```powershell
# Add Qt to CMAKE_PREFIX_PATH
[System.Environment]::SetEnvironmentVariable("CMAKE_PREFIX_PATH", "C:\Qt\6.5.3\msvc2019_64", "User")
```
Restart terminal after setting.

**Method 2: CMake Command Line (Per-build)**
```powershell
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64"
```

**Method 3: Use Qt's CMake**
```powershell
# Use Qt's bundled CMake
C:\Qt\6.5.3\msvc2019_64\bin\qt-cmake.bat ..
```

## 🏗️ Build Instructions

### Full Build Process

```powershell
# 1. Navigate to project
cd "c:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final"

# 2. Create build directory
New-Item -ItemType Directory -Path build -Force
cd build

# 3. Configure (choose one method)
# Method A: If CMAKE_PREFIX_PATH is set
cmake .. -G "Visual Studio 17 2022" -A x64

# Method B: Specify Qt path directly
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\msvc2019_64"

# Method C: Use Qt's CMake
C:\Qt\6.5.3\msvc2019_64\bin\qt-cmake.bat .. -G "Visual Studio 17 2022" -A x64

# 4. Build
cmake --build . --config Release

# 5. Run
.\bin\Release\HeatXTwin_Pro.exe
```

### Quick Rebuild
```powershell
cd build
cmake --build . --config Release
```

## 🎯 Alternative: Qt Creator

If you prefer using Qt Creator IDE:

1. Open Qt Creator
2. File → Open File or Project
3. Select `CMakeLists.txt` from HeatXTwin_Final folder
4. Configure build kit (select MSVC2019 64-bit or 2022)
5. Click "Build" (Ctrl+B)
6. Click "Run" (Ctrl+R)

## 🐛 Troubleshooting

### "Could not find Qt6"

**Solution 1**: Verify Qt installation
```powershell
# Check if Qt is installed
Test-Path "C:\Qt\6.5.3\msvc2019_64\lib\cmake\Qt6"
```

**Solution 2**: Set CMAKE_PREFIX_PATH correctly
```powershell
# The path should point to the MSVC directory, not the base Qt folder
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.5.3\msvc2019_64"
```

**Solution 3**: Use absolute path in cmake command
```powershell
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"
```

### "Qt Charts not found"

Ensure Qt Charts module is installed:
- Re-run Qt Maintenance Tool
- Select "Add or remove components"
- Find "Qt Charts" under your Qt version
- Install if missing

### "MSVC compiler not found"

**Solution**: Open **Developer Command Prompt** instead of regular PowerShell:
1. Start Menu → "Developer Command Prompt for VS 2022"
2. Or use: "x64 Native Tools Command Prompt for VS 2022"
3. Run build commands from there

### Build succeeds but executable crashes

**Solution**: Copy Qt DLLs
```powershell
# Copy required Qt DLLs to build output
cd build\bin\Release
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Core.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Gui.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Widgets.dll .
copy C:\Qt\6.5.3\msvc2019_64\bin\Qt6Charts.dll .
```

**Better Solution**: Use `windeployqt`
```powershell
cd build\bin\Release
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe HeatXTwin_Pro.exe
```

## 📦 Creating Standalone Distribution

To create a portable version:

```powershell
# 1. Build in Release mode
cmake --build build --config Release

# 2. Create distribution folder
New-Item -ItemType Directory -Path dist -Force

# 3. Copy executable
copy build\bin\Release\HeatXTwin_Pro.exe dist\

# 4. Deploy Qt dependencies
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe dist\HeatXTwin_Pro.exe

# 5. Copy config files
copy configs\baseline.toml dist\configs\

# 6. Copy VC++ redistributables (if needed)
# Download from: https://aka.ms/vs/17/release/vc_redist.x64.exe
```

Now `dist\` folder can be zipped and shared!

## ✅ Verification

After successful build, verify:

```powershell
# Check executable exists
Test-Path build\bin\Release\HeatXTwin_Pro.exe

# Check config file copied
Test-Path build\configs\baseline.toml

# Run application
.\build\bin\Release\HeatXTwin_Pro.exe
```

Expected behavior:
- ✅ Window opens with modern UI
- ✅ 4 chart tabs visible
- ✅ Left panel with parameter controls
- ✅ START button is green and visible
- ✅ Status shows "Ready"

## 🚀 Next Steps

Once built successfully:
1. Click START to run a simulation with defaults
2. Watch charts populate in real-time
3. Try modifying parameters and re-running
4. Switch between chart tabs during simulation
5. Experiment with Reset button

## 📝 Common Qt Installation Paths

Check these locations:
- `C:\Qt\6.5.3\msvc2019_64`
- `C:\Qt\6.6.0\msvc2022_64`
- `C:\Program Files\Qt\6.5.3\msvc2019_64`
- `C:\Users\<YourName>\Qt\6.5.3\msvc2019_64`

For vcpkg:
- `C:\vcpkg\installed\x64-windows`

---

**Need Help?**  
Check Qt documentation: https://doc.qt.io/qt-6/  
CMake documentation: https://cmake.org/documentation/
