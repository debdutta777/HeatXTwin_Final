# Quick Start Guide - Running Locally

## 🚀 How to Start the Application

### Method 1: Using the Launch Script (EASIEST)

Double-click: **`run_app.bat`** in the project root folder

### Method 2: Direct Execution

Navigate to the build directory and run the executable:

```
C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release\HeatXTwin_Pro.exe
```

### Method 3: From Command Line

```cmd
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final"
run_app.bat
```

OR

```cmd
cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final\build\bin\Release"
HeatXTwin_Pro.exe
```

---

## 📁 Application Locations

### Executable Location:
```
HeatXTwin_Final\build\bin\Release\HeatXTwin_Pro.exe
```

### Configuration Files:
```
HeatXTwin_Final\configs\baseline.toml
```

### All Required DLLs (automatically in same folder):
- Qt6 libraries (Qt6Core.dll, Qt6Gui.dll, etc.)
- ICU data files
- Platform plugins (platforms\qwindows.dll)
- Style plugins (styles\qmodernwindowsstyle.dll)

---

## 🔧 First Time Setup

If you haven't built the application yet:

1. **Build using CMake:**
   ```cmd
   cd "C:\Users\DEBDUTTA\Desktop\MAJOR PROJECT\HeatXTwin_Final"
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```

2. **Then run:**
   ```cmd
   run_app.bat
   ```

---

## ⚙️ Using the Application

1. **Start Simulation:**
   - Click the green "Start" button
   - Adjust parameters in real-time
   - Use speed multiplier dropdown (1×-100×)

2. **View Results:**
   - Monitor charts for temperature, pressure, and fouling
   - Hover over chart points to see exact values
   - Watch real-time data updates

3. **Control Simulation:**
   - Green button = Start
   - Yellow button = Pause
   - Red button = Stop

---

## 🐛 Troubleshooting

### Application won't start:
- Check all DLLs are in the `build\bin\Release` folder
- Verify the build completed successfully
- Look for error messages in the console

### Missing DLLs:
- Rebuild the project: `cmake --build build --config Release`
- Check vcpkg installed Qt6 correctly

### Configuration errors:
- Ensure `configs\baseline.toml` exists
- Check file permissions

---

## 📚 More Information

- Full documentation: See `TECHNICAL_DOCUMENTATION.md`
- Build guide: See `BUILD_GUIDE.md`
- Setup instructions: See `SETUP_GUIDE.md`
