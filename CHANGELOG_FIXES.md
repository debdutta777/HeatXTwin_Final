# Fixes and Improvements

## 1. Fluid Assignment Correction
- **Issue:** Report specifies Hot fluid in Tubes ($Re_h$) and Cold fluid in Shell ($Re_c$), but code had them swapped.
- **Fix:** Swapped fluid properties in `Thermo` and `Hydraulics` classes.
- **Details:**
  - `Thermo::h_tube` now uses `hot` fluid properties.
  - `Thermo::h_shell` now uses `cold` fluid properties.
  - `Hydraulics::dP_tube` now uses `hot` fluid properties.
  - `Hydraulics::dP_shell` now uses `cold` fluid properties.
  - Updated Dittus-Boelter exponent to 0.3 (cooling) for tube side.

## 2. Shell-Side Pressure Drop Correction
- **Issue:** Shell-side $\Delta P$ used empirical multiplier instead of physical diameter reduction.
- **Fix:** Updated `Hydraulics::dP_shell` to calculate effective hydraulic diameter ($D_{e,eff}$) based on fouling thickness.
- **Details:**
  - Calculated `De_eff = De_clean - 2 * delta_shell`.
  - Recalculated Flow Area and Velocity based on `De_eff`.
  - Recalculated Reynolds Number.
  - Used standard friction factor.

## 3. Hardcoded Parameters
- **Issue:** `k_deposit`, `K_minor`, `K_turns` were hardcoded.
- **Fix:** Exposed these parameters in `Types.hpp` and passed them through `Simulator`.
- **Details:**
  - Added `k_deposit` and `split_ratio` to `FoulingParams`.
  - Added `K_minor_tube` and `K_turns_shell` to `Geometry`.
  - Updated `Thermo` and `Hydraulics` methods to accept these parameters.
  - Updated `Simulator` to pass these parameters from configuration.

## 4. Fouling Distribution
- **Issue:** Fouling split was hardcoded to 50/50.
- **Fix:** Added `split_ratio` to `FoulingParams`.
- **Details:**
  - `Simulator::step` now uses `split_ratio` to distribute total fouling resistance between shell and tube sides.

## 5. Dynamic Disturbances
- **Issue:** Disturbances were hardcoded sine waves.
- **Fix:** Added `DisturbanceType` to `SimConfig`.
- **Details:**
  - Supported types: `SineWave`, `StepChange`, `Ramp`, `None`.
  - `Simulator::step` switches on `disturbanceType`.

## 6. Chart Visualization
- **Issue:** Fouling chart auto-scaling was noisy for small values.
- **Fix:** Improved auto-scaling logic in `ChartWidget.cpp`.
- **Details:**
  - Increased `maxY_` for Fouling chart.
  - Added minimum range enforcement for Fouling chart to prevent noise amplification.
