# Quick Build Guide - HeatXTwin Pro 2.0

## ✅ What's Already Set Up

The project is now configured to use **vcpkg** (same as the original HeatXTwin project) for dependency management. This means:

- ✅ Qt6 will be automatically downloaded and built
- ✅ All other dependencies (Eigen3, fmt, spdlog, tomlplusplus) handled automatically
- ✅ No manual Qt installation needed
- ✅ Everything is self-contained

## 🚀 Building the Project

### First Time Setup (IN PROGRESS)

```powershell
# 1. Configure with CMake (this is currently running!)
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build"
cmake -S .. -B . -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:\Users\DEBDUTTA\vcpkg\scripts\buildsystems\vcpkg.cmake"

# This step takes 5-15 minutes as vcpkg builds Qt6 and dependencies
# Wait for "-- Configuring done" message
```

### After Configuration Completes

```powershell
# 2. Build the application
cmake --build . --config Release

# 3. Run it!
.\bin\Release\HeatXTwin_Pro.exe
```

## 📦 What vcpkg is Installing

Currently building:
- **qtbase** - Core Qt6 framework (widgets, GUI, network, etc.)
- **qtcharts** - Qt Charts module for our 4 specialized charts
- **eigen3** - Linear algebra library (for simulation math)
- **fmt** - Fast string formatting
- **spdlog** - Logging library  
- **tomlplusplus** - TOML configuration file parsing
- Plus ~30 dependencies (OpenSSL, ICU, FreeType, libjpeg, libpng, etc.)

## ⏱️ Build Times

- **First build**: 10-20 minutes (vcpkg + compilation)
- **Incremental builds**: 30 seconds - 2 minutes
- **Clean rebuilds**: 2-5 minutes (just recompile, vcpkg cache used)

## 🔄 Subsequent Builds

After the first successful build, you only need:

```powershell
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build"
cmake --build . --config Release
.\bin\Release\HeatXTwin_Pro.exe
```

## 🐛 If Build Fails

### vcpkg errors
```powershell
# Update vcpkg
cd C:\Users\DEBDUTTA\vcpkg
git pull
.\bootstrap-vcpkg.bat
```

### CMake cache issues
```powershell
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build"
Remove-Item * -Recurse -Force
# Then re-run cmake command
```

### Compiler not found
Open **x64 Native Tools Command Prompt for VS 2022** instead of regular PowerShell

## ✨ What You'll Get

Once built, you'll have:
- ✅ **HeatXTwin_Pro.exe** - Modern, professional UI application
- ✅ **4 optimized charts** - Temperature, Heat/U, Pressure, Fouling
- ✅ **Live parameter editing** - No config file editing needed
- ✅ **Background threading** - Responsive UI during simulation
- ✅ **Auto-scaling charts** - Adaptive axes for perfect visualization
- ✅ **Modern styling** - Professional gradients and colors

## 📊 Expected UI

```
┌─────────────────────────────────────────────────────────────────┐
│ ▶ START  ■ STOP  🔄 Reset  💾 Save  📁 Load  Duration: [1800s] │
├──────────────┬──────────────────────────────────────────────────┤
│ 🌡️ Operating │ [🌡️ Temperatures][🔥 Heat & U][💧 Pressure][⚙️ Fouling]│
│ Conditions   │                                                  │
│ • Hot Flow   │           (Auto-scaling charts)                  │
│ • Cold Flow  │       (Real-time data updates)                   │
│ • Temps      │     (4 separate optimized views)                 │
│              │                                                  │
│ 📐 Geometry  │                                                  │
│ • Tubes      │                                                  │
│ • Diameter   │                                                  │
│ • Length     │                                                  │
│ • Shell      │                                                  │
│ • Baffles    │                                                  │
│              │                                                  │
│ ⚙️ Fouling   │                                                  │
│ • Rf Max     │                                                  │
│ • Tau        │                                                  │
└──────────────┴──────────────────────────────────────────────────┘
```

---

**Status**: vcpkg is currently building Qt6 and dependencies...  
**Progress**: Check terminal output for build progress  
**ETA**: 5-15 minutes until ready to build application
