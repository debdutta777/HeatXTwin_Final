# HeatXTwin Pro 2.0 - Modern Heat Exchanger Digital Twin

**Complete redesign** with professional UI, multiple optimized charts, and real-time parameter editing.

## ✨ Features

### 🎨 Modern Interface
- **Clean tabbed layout** with 4 specialized charts
- **Professional styling** with gradients and rounded corners
- **Live parameter editing** - no config file editing required
- **Responsive design** with adjustable panels

### 📊 Advanced Visualization
- **Temperature Chart** (20-100°C range): Hot/Cold outlet temperatures
- **Heat Duty & U Chart** (0-500 range): Heat transfer rate and overall coefficient
- **Pressure Drop Chart** (0-40kPa range): Tube-side and shell-side pressure drops
- **Fouling Chart** (0-10 range): Fouling resistance over time
- **Auto-scaling axes** with adaptive margins every 10 samples
- **Color-coded series** for easy identification

### 🔧 Editable Parameters

#### Operating Conditions
- Hot & Cold flow rates (0.1-5.0 kg/s)
- Hot inlet temperature (40-150°C)
- Cold inlet temperature (5-50°C)

#### Geometry
- Number of tubes (50-500)
- Tube inner diameter (10-50 mm)
- Tube length (1-20 m)
- Shell inner diameter (200-1000 mm)
- Baffle spacing (100-1000 mm)
- Number of baffles (5-50)

#### Fouling Model
- Maximum fouling resistance (0-0.01 m²K/W)
- Time constant τ (300-10000 s)

### ⚙️ Simulation Control
- **START/STOP** buttons with visual feedback
- **Duration** control (100-10000 s)
- **Time step** adjustment (0.1-10 s)
- **Reset** to default values
- **Save/Load** configuration files (TOML)

### 🧵 Performance
- **Background threading** - UI remains responsive during simulation
- **Real-time updates** with progress tracking
- **No freezing** even during long simulations

## 🏗️ Architecture

```
HeatXTwin_Final/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── core/                       # Verified simulation engine
│   │   ├── Thermo.cpp             # Heat transfer calculations
│   │   ├── Hydraulics.cpp         # Pressure drop & flow
│   │   ├── Fouling.cpp            # Fouling models
│   │   ├── Simulator.cpp          # Time-stepping simulation
│   │   ├── Model.cpp              # State-space model
│   │   ├── ControllerPID.cpp      # PID controller
│   │   ├── EstimatorRLS.cpp       # RLS estimator
│   │   └── Validation.cpp         # Data validation
│   ├── app/ui/                     # Modern UI components
│   │   ├── MainWindow.hpp/cpp     # Main application window
│   │   ├── ChartWidget.hpp/cpp    # Specialized chart widget
│   │   └── SimWorker.hpp/cpp      # Background simulation thread
│   └── io/                         # Configuration I/O
│       └── ConfigLoader.cpp        # TOML file handling
├── configs/
│   └── baseline.toml               # Default configuration
├── CMakeLists.txt                  # Build configuration
└── README.md                       # This file
```

## 🚀 Building

### Prerequisites
- CMake 3.16+
- Qt 6.x (Core, Widgets, Charts)
- C++17 compiler (MSVC, GCC, Clang)

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS
```bash
mkdir build
cd build
cmake ..
make -j4
```

## ▶️ Running

### Windows
```bash
.\build\bin\Release\HeatXTwin_Pro.exe
```

### Linux/macOS
```bash
./build/bin/HeatXTwin_Pro
```

## 📖 Usage Guide

### Quick Start
1. **Launch** the application
2. **Review** default parameters in the left panel
3. **Adjust** any parameters as needed (live editing)
4. **Click START** to begin simulation
5. **Switch tabs** to view different charts
6. **Click STOP** to halt early (optional)
7. **Reset** to restore defaults

### Best Practices
- **Start simple**: Use default values first to understand behavior
- **One change**: Modify one parameter at a time to see effects
- **Monitor all charts**: Switch between tabs during simulation
- **Save configs**: Export working parameter sets for reuse
- **Longer durations**: Use 3600s+ to see fouling effects clearly

### Parameter Guidelines
- **Flow rate balance**: Keep hot/cold flows within 0.5-2.0 ratio
- **Temperature difference**: Maintain ΔT > 20°C for effective heat transfer
- **Baffle spacing**: Generally 0.2-0.5 × shell diameter
- **Tube count**: Scale with desired capacity (more tubes = more Q)
- **Fouling time constant**: 1800s (30 min) typical for moderate fouling

## 🔬 Physical Models

### Heat Transfer
- **Nusselt correlations** for tube & shell side
- **ε-NTU method** for counter-flow effectiveness
- **Overall U coefficient** with fouling resistance
- **Dynamic energy balance** for transient response

### Pressure Drop
- **Darcy-Weisbach** for tube-side friction
- **Kern-style correlation** for shell-side pressure drop
- **Minor losses** at inlet/outlet

### Fouling
- **Asymptotic model**: Rf(t) = Rf_max × (1 - exp(-t/τ))
- **Linear model**: Rf(t) = α × t
- **Dynamic update** at each time step

### Real-Time Simulation
- **1:1 time mapping**: 1800s simulation = 1800s real time
- **Dynamic disturbances**: Multi-frequency temperature & flow variations
- **Thermal inertia**: Realistic time constants (90-180s)

## 🎯 Validation

All mathematical formulas have been **verified correct** through comprehensive audit:
- ✅ Reynolds, Prandtl, Nusselt numbers
- ✅ Heat transfer coefficients (tube & shell)
- ✅ Overall U calculation with wall & fouling resistance
- ✅ ε-NTU effectiveness and heat duty balance
- ✅ Pressure drop calculations (Darcy-Weisbach, Kern method)
- ✅ Fouling model integration
- ✅ Energy conservation checks (<1% error)

**For complete formula verification and implementation details, see:**
- 📘 [`TECHNICAL_DOCUMENTATION.md`](TECHNICAL_DOCUMENTATION.md) - Comprehensive technical reference (15,000+ words)
- 📊 [`EXECUTIVE_SUMMARY.md`](EXECUTIVE_SUMMARY.md) - Business case and roadmap

## 🐛 Troubleshooting

### Charts not updating
- Ensure simulation is running (status shows "Running")
- Check that duration > 0 and time step is reasonable
- Try resetting and restarting

### Parameter changes not applied
- Changes only apply to **new simulations** (not during run)
- Click START again after modifying parameters

### Build errors
- Verify Qt6 is installed and CMAKE_PREFIX_PATH is set
- Check C++17 compiler support
- Ensure all dependencies are available

### Performance issues
- Reduce time step for faster execution
- Lower number of tubes for lighter computation
- Close other applications

## 📝 License

Academic/Research Use

## 👥 Authors

Digital Twin Lab - Heat Exchanger Modeling Team

## 📧 Support

For issues or questions about the simulation models, refer to source code documentation in `src/core/` headers.

---

**Version**: 2.0  
**Last Updated**: 2024  
**Status**: Production Ready ✅
