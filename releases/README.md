# HeatXTwin_Final - Releases

## Latest Release: v1.0.0

### Download and Installation

#### For Windows (64-bit)

**⚠️ IMPORTANT: Download BOTH files**

**Quick Start:**
1. Download the Core package: `HeatXTwin_v1.0.0_Core.zip` (~15 MB)
2. Download the Data package: `HeatXTwin_v1.0.0_Data.zip` (~12 MB)
3. Extract **BOTH** ZIP files to the **SAME** folder
4. Run `HeatXTwin_Pro.exe`

> **Note:** Both files are required. The application splits the package to meet GitHub's file size limits.

**System Requirements:**
- Windows 10/11 (64-bit)
- 4 GB RAM minimum (8 GB recommended)
- ~500 MB disk space

### What's Included

The release package includes:
- `HeatXTwin_Pro.exe` - Main application
- All required Qt6 DLLs and dependencies
- Configuration files in `configs/` folder
- Sample baseline configuration: `baseline.toml`

### First Run

1. Launch `HeatXTwin_Pro.exe`
2. Use the default configuration or modify parameters
3. Click "Start" (green button) to begin simulation
4. Use speed multiplier (1×-100×) to control simulation speed
5. Hover over charts to see real-time data values

### Features

- **Real-time Simulation**: Runs at actual time scale (with speed control)
- **Interactive Charts**: Temperature, pressure, and fouling trends
- **Speed Control**: 1×, 2×, 5×, 10×, 20×, 50×, 100× multipliers
- **Intuitive Controls**: Color-coded buttons (Green=Start, Yellow=Pause, Red=Stop)
- **Live Data Display**: Hover tooltips on all charts

### Troubleshooting

**Application won't start:**
- Ensure all DLL files are in the same directory as the .exe
- Install Visual C++ Redistributable if prompted
- Check Windows Defender hasn't blocked the executable

**Performance issues:**
- Close other applications to free up memory
- Reduce simulation speed multiplier
- Check Task Manager for CPU/Memory usage

### Building from Source

See [BUILD_GUIDE.md](../BUILD_GUIDE.md) for instructions on building from source code.

### Support

For issues, questions, or feature requests:
- Open an issue on GitHub
- Check [TECHNICAL_DOCUMENTATION.md](../TECHNICAL_DOCUMENTATION.md) for details
- See [QUICK_REFERENCE.md](../QUICK_REFERENCE.md) for usage guide

---

**Version History:**

- **v1.0.0** (October 2025) - Initial Release
  - Complete heat exchanger simulation
  - Real-time visualization
  - Speed control feature
  - Interactive charts with tooltips
  - Comprehensive documentation
