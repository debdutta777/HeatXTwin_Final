# Fixes Report - Codebase Inconsistencies

The following issues identified in the codebase vs. report comparison have been resolved:

### 1. Critical Physics Error: Wall Resistance Formula
- **Issue:** The wall resistance formula in `Thermo.cpp` was dividing by an extra `Di` term.
- **Fix:** Corrected the formula to: `Rw = (Do * ln(Do/Di)) / (2 * k_wall)`.

### 2. Shell-Side Flow Area Inconsistency
- **Issue:** `Thermo.cpp` and `Hydraulics.cpp` used a simplified shell area formula (`PI * Ds * B`).
- **Fix:** Updated both files to use the Kern method formula: `As = (Ds / Pt) * (Pt - Do) * B`.

### 3. Nusselt Number Exponent Contradiction
- **Issue:** Report text says n=0.4 (heating), but code uses n=0.3 (cooling).
- **Resolution:** Kept n=0.3 as it is physically correct for the current setup (Hot fluid in tubes = Cooling). Added a comment in `Thermo.cpp` to clarify the discrepancy with the report text.

### 4. Hardcoded Simulation Disturbances
- **Issue:** Disturbance parameters were hardcoded in `Simulator.cpp`.
- **Fix:** Moved disturbance parameters (amplitudes, frequencies, step times) to `SimConfig` struct in `Simulator.hpp`. `Simulator.cpp` now uses these configurable values.

### 5. Implementation Inconsistency in Fouling Module
- **Issue:** `Fouling::thickness` ignored `p_.k_deposit` and used a hardcoded value.
- **Fix:** Updated `Fouling.cpp` to use `p_.k_deposit`.

### 6. Redundant Geometry Data
- **Issue:** `wall_thickness` in `Geometry` was potentially redundant or ignored.
- **Resolution:** The wall resistance calculation (`Rw`) now correctly uses `Do` and `Di` as the primary geometric inputs, ensuring consistency with the physical dimensions.

## Verification
The project has been successfully rebuilt with these changes.
