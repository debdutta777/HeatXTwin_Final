# HeatXTwin Pro 2.0 - Quick Reference Guide

## 🚀 Getting Started in 5 Minutes

### 1. Launch Application
**Windows**: Double-click `HeatXTwin_Pro.exe`  
**Linux/Mac**: Run `./HeatXTwin_Pro`

### 2. Review Default Parameters (Left Panel)
The application opens with realistic default values:
- Hot flow: 1.2 kg/s, 80°C inlet
- Cold flow: 1.0 kg/s, 25°C inlet
- 100 tubes, 5m long, 500mm shell

### 3. Click Green "START" Button
- Simulation runs in real-time (1800s = 30 real minutes)
- Watch 4 charts update live
- Hover mouse over lines to see exact values

### 4. Switch Between Chart Tabs
- **Temperatures**: Hot & cold outlet temperatures
- **Heat & U**: Heat duty and overall transfer coefficient
- **Pressures**: Tube-side and shell-side pressure drops
- **Fouling**: Fouling resistance growth over time

### 5. Export Data
- Click **"Export CSV"** button after simulation completes
- Open in Excel/MATLAB for further analysis

---

## 🎨 Understanding the Interface

### Color Coding
- **Green Button (START)**: Begin simulation
- **Yellow Button (PAUSE)**: Pause/resume simulation
- **Red Button (STOP)**: Stop simulation completely
- **Blue Headers**: Parameter group titles

### Chart Controls
- **Mouse Wheel**: Zoom in/out
- **Space + Drag**: Pan around
- **Hover**: See exact time and value
- **Zoom In/Out/Reset Buttons**: Top right toolbar

### Status Bar (Top)
- Shows "Ready", "Running X%", or "Finished"
- Progress percentage during simulation

---

## 📐 Parameter Guide

### Operating Conditions

| Parameter | Typical Range | Default | Effect |
|-----------|---------------|---------|--------|
| **Hot Flow Rate** | 0.5-2.0 kg/s | 1.2 kg/s | Higher flow → More heat transfer, higher pressure drop |
| **Cold Flow Rate** | 0.5-2.0 kg/s | 1.0 kg/s | Higher flow → More heat transfer, higher pressure drop |
| **Hot Inlet Temp** | 60-120°C | 80°C | Higher temp → More heat transferred |
| **Cold Inlet Temp** | 10-40°C | 25°C | Lower temp → More heat transferred |

**Rule of Thumb**: Keep ΔT_inlet (hot - cold) > 20°C for efficient operation

---

### Geometry Parameters

| Parameter | Typical Range | Default | Effect |
|-----------|---------------|---------|--------|
| **Number of Tubes** | 50-200 | 100 | More tubes → More heat transfer area |
| **Tube Inner Diameter** | 15-25 mm | 19 mm | Larger → Lower pressure drop, lower h |
| **Tube Length** | 3-8 m | 5 m | Longer → More heat transfer |
| **Shell Diameter** | 400-800 mm | 500 mm | Larger → Lower shell pressure drop |
| **Baffle Spacing** | 200-500 mm | 300 mm | Closer → Higher h_shell, higher ΔP |
| **Number of Baffles** | 10-20 | 15 | More → Better mixing, higher ΔP |

**Rule of Thumb**: Baffle spacing ≈ 0.4 × Shell diameter

---

### Fouling Model

| Parameter | Typical Range | Default | Effect |
|-----------|---------------|---------|--------|
| **Initial Fouling** | 0 | 0.0001 m²K/W | Starting cleanliness |
| **Max Fouling** | 0.0003-0.001 | 0.0005 m²K/W | Asymptotic limit |
| **Time Constant τ** | 1800-5400 s | 1800 s | How fast fouling grows |

**Interpretation**:
- τ = 1800s (30 min): Rapid fouling (dirty fluid)
- τ = 3600s (1 hr): Moderate fouling (typical water)
- τ = 7200s (2 hr): Slow fouling (clean fluid)

**Fouling reaches 95% of maximum in 3×τ**

---

### Fluid Properties (Water Defaults)

| Property | Hot Fluid | Cold Fluid | Units |
|----------|-----------|------------|-------|
| **Density** | 980 | 998 | kg/m³ |
| **Viscosity** | 0.0004 | 0.001 | Pa·s |
| **Specific Heat** | 4180 | 4180 | J/kg/K |
| **Conductivity** | 0.65 | 0.60 | W/m/K |

**Note**: Values are constant (not temperature-dependent in v2.0)

---

## 📊 Interpreting Results

### Temperature Chart
- **Cold outlet going up** = Fouling increasing (less efficient)
- **Hot outlet going up** = Fouling increasing
- **Oscillations** = Normal (represents process disturbances)

### Heat & U Chart
- **Q (Heat Duty)** should be 50-200 kW for typical defaults
- **U (Overall coefficient)** should be 800-1200 W/m²K when clean
- **U decreasing** = Fouling accumulating
- **When U drops 30%**, consider cleaning

### Pressure Chart
- **Tube-side ΔP** typically 10-30 kPa
- **Shell-side ΔP** typically 5-20 kPa
- **ΔP increasing** = Fouling reducing flow area
- **If ΔP exceeds 40 kPa (tube) or 30 kPa (shell)**, red flag for cleaning

### Fouling Chart
- **Starts near 0.0001** m²K/W (clean)
- **Grows toward 0.0005** m²K/W (target max)
- **Curve flattens** = Approaching equilibrium
- **If Rf > 0.0007**, definitely time to clean

---

## 🎯 Common Scenarios

### Scenario 1: Increase Heat Transfer
**Goal**: Transfer more heat (higher Q)

**Try**:
1. Increase hot inlet temperature (+10°C)
2. Increase flow rates (+20%)
3. Add more tubes (+20)
4. Increase tube length (+1m)

**Tradeoff**: Higher ΔP (more pumping cost)

---

### Scenario 2: Reduce Pressure Drop
**Goal**: Lower pumping costs

**Try**:
1. Increase tube diameter (+2mm)
2. Reduce tube length (-1m)
3. Reduce number of baffles (-3)
4. Increase baffle spacing (+50mm)

**Tradeoff**: Lower heat transfer (lower U)

---

### Scenario 3: Predict Cleaning Schedule
**Goal**: Know when to clean based on fouling

**Steps**:
1. Set realistic fouling parameters (τ = 3600s, Rf_max = 0.0005)
2. Run simulation for 10,000s (2.8 hours real time)
3. Note when U drops below 80% of initial value
4. Scale to real plant: If U drops 20% in 2.8hrs simulated...
   - And your real time constant is 30 days (not 1 hr)
   - Then cleaning needed in: 2.8 hrs × (30 days / 1 hr) = 2016 hours ≈ 84 days

---

### Scenario 4: Operator Training
**Goal**: Show what happens if cold flow stops

**Steps**:
1. Start simulation normally
2. After 600s, pause simulation
3. Reduce cold flow to 0.1 kg/s (near-zero)
4. Resume simulation
5. **Observe**: Hot outlet temp rises (less cooling)

**Learning**: Importance of maintaining flow

---

## ⚙️ Advanced Features

### Zoom & Pan (Charts)
- **Zoom In**: Mouse wheel up or click "Zoom In" button
- **Zoom Out**: Mouse wheel down or click "Zoom Out" button
- **Pan**: Hold Space, then drag mouse
- **Reset**: Click "Reset Zoom" button

### Pause & Resume
- **Pause**: Yellow button (simulation continues in background, updates stop)
- **Resume**: Click yellow button again
- **Use case**: Pause to examine current state, then continue

### Export Data
- **Format**: CSV (comma-separated values)
- **Columns**: Time, Tc_out, Th_out, Q, U, Rf, dP_tube, dP_shell
- **Location**: Same folder as executable, filename: `simulation_data.csv`
- **Analysis**: Open in Excel, plot in MATLAB/Python

---

## 🔧 Troubleshooting

### Problem: Charts Not Updating
**Solution**:
- Check status bar says "Running"
- Verify duration > 0 and time step > 0
- Try clicking STOP, then START again

### Problem: Simulation Too Slow
**Cause**: Real-time execution (1800s takes 30 minutes)

**Solutions**:
- Reduce duration (try 600s = 10 minutes)
- Increase time step from 1.0s to 2.0s (fewer steps)
- Note: Future versions may have "fast mode"

### Problem: Simulation Too Fast
**Cause**: Real-time delay working correctly

**Action**: Intended behavior - enjoy the smooth animation!

### Problem: Values Look Wrong
**Check**:
- Fluid properties match your fluid (not all fluids are water)
- Temperature units are °C (not °F or K)
- Flow rates in kg/s (not L/min or GPM)
- Diameter in mm (not inches)

**Conversion**:
- GPM → kg/s: multiply by 0.063 (for water)
- Inches → mm: multiply by 25.4
- °F → °C: (°F - 32) × 5/9

### Problem: Chart Says "No Data"
**Cause**: Simulation not started yet

**Solution**: Click green START button

---

## 📏 Units Reference

| Quantity | Unit | Notes |
|----------|------|-------|
| Temperature | °C | Celsius (not Fahrenheit) |
| Flow Rate | kg/s | Mass flow (not volumetric) |
| Length | m | Meters for length |
| Diameter | mm | Millimeters for tubes/shell |
| Pressure Drop | Pa | Pascals (1 bar = 100,000 Pa) |
| Heat Duty | W | Watts (1 kW = 1000 W) |
| Heat Transfer Coeff | W/m²/K | - |
| Fouling Resistance | m²K/W | - |
| Density | kg/m³ | - |
| Viscosity | Pa·s | Dynamic viscosity |
| Specific Heat | J/kg/K | - |
| Conductivity | W/m/K | Thermal conductivity |

---

## 🆘 Getting Help

### Documentation
1. **This Guide** - Quick reference
2. **README.md** - Feature overview and build instructions
3. **TECHNICAL_DOCUMENTATION.md** - Complete formula verification (15,000 words)
4. **EXECUTIVE_SUMMARY.md** - Business case and roadmap

### Online Resources
- **Heat Transfer Textbook**: Incropera & DeWitt (fundamentals)
- **TEMA Standards**: Shell-and-tube design standards
- **YouTube**: Search "shell and tube heat exchanger" for visual guides

### Support
- **Email**: [Contact Information]
- **Issues**: Report bugs via project repository
- **Training**: Contact for custom training sessions

---

## ✅ Validation Checklist

Before trusting results for critical decisions:

- [ ] Fluid properties match your actual fluid
- [ ] Geometry parameters match your actual heat exchanger
- [ ] Operating conditions are realistic (flow rates, temperatures)
- [ ] Results are physically sensible (T_out between T_in values)
- [ ] Energy balance closes (Q_hot ≈ Q_cold)
- [ ] Pressure drops are reasonable (<100 kPa typically)
- [ ] Fouling time constants match your experience

**If in doubt**: Start with known cases and compare to plant data

---

## 🎓 Best Practices

### For Training
1. Start with defaults to show normal operation
2. Make one change at a time
3. Explain cause-and-effect
4. Use pause to discuss current state
5. Export data for post-training review

### For Design Studies
1. Run baseline case first
2. Document all parameter changes
3. Run multiple scenarios (optimistic, nominal, pessimistic)
4. Export all results for comparison
5. Present tradeoffs (heat transfer vs. pressure drop)

### For Research
1. Verify formulas in TECHNICAL_DOCUMENTATION.md
2. Run sensitivity analysis (vary each parameter ±10%)
3. Compare with literature correlations
4. Document assumptions and limitations
5. Cite properly in publications

---

## 📅 Typical Simulation Duration Guide

| Real Time | Sim Duration | Use Case |
|-----------|--------------|----------|
| 5 seconds | 5s | Test changes |
| 1 minute | 60s | Quick check |
| 5 minutes | 300s | Startup dynamics |
| 10 minutes | 600s | Short-term fouling |
| 30 minutes | 1800s | Default setting |
| 1 hour | 3600s | Medium-term fouling |
| 2 hours | 7200s | Long-term trends |

**Note**: Real-time execution means you wait the actual duration!

---

## 🔮 What's Coming Next

### Version 2.1 (Q1 2026)
- Save/load custom presets
- Fluid library (10 common fluids)
- Faster-than-real-time mode

### Version 2.5 (Q2 2026)
- Bell-Delaware shell-side model
- Multi-pass configurations
- Temperature-dependent properties

### Version 3.0 (Q4 2026)
- OPC UA plant connectivity
- Predictive maintenance alerts
- Web dashboard

**Want to influence the roadmap?** Contact us with your needs!

---

## 💡 Pro Tips

1. **Start Small**: Test with 600s duration before running 3600s
2. **Document Changes**: Keep notes on what you change and why
3. **Compare Cases**: Run baseline, then one change - easier to see effect
4. **Use Export**: Analyze data offline for better insights
5. **Learn the Physics**: Read TECHNICAL_DOCUMENTATION.md to understand "why"

---

## 📖 Glossary

**ΔP (Delta-P)**: Pressure drop (Pa)  
**ΔT (Delta-T)**: Temperature difference (°C)  
**ε (Epsilon)**: Effectiveness (dimensionless, 0-1)  
**NTU**: Number of Transfer Units (dimensionless)  
**Q**: Heat duty (W or kW)  
**U**: Overall heat transfer coefficient (W/m²/K)  
**Rf**: Fouling resistance (m²K/W)  
**Re**: Reynolds number (dimensionless, flow regime indicator)  
**Pr**: Prandtl number (dimensionless, fluid property ratio)  
**Nu**: Nusselt number (dimensionless, convection indicator)  
**LMTD**: Log Mean Temperature Difference (°C)  
**Tc_out**: Cold fluid outlet temperature (°C)  
**Th_out**: Hot fluid outlet temperature (°C)  
**h_tube**: Tube-side heat transfer coefficient (W/m²/K)  
**h_shell**: Shell-side heat transfer coefficient (W/m²/K)  

---

**Quick Reference Guide Version 1.0**  
**Last Updated**: October 9, 2025  
**For HeatXTwin Pro 2.0**

*Keep this guide handy while using the simulator!*
