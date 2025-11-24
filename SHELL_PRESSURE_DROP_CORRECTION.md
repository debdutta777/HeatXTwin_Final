# Shell-Side Pressure Drop Correction - Alignment with Report

**Date:** November 24, 2025  
**Status:** ✅ COMPLETED

---

## Overview

The shell-side pressure drop calculation has been updated to align with the report's theoretical model, replacing the previous empirical approximation.

**Report Specification:**
> "Fouling changes the shell-side hydraulic diameter ($D_{e,s,f}$) which should be used to recalculate the Reynolds number and velocity, thereby affecting the friction factor and $\Delta P_{shell}$."

**Previous Code Implementation:**
- Used clean equivalent diameter ($D_e$) for velocity and Reynolds number.
- Applied an empirical multiplier to the friction factor: `f = f_clean * (1.0 + 5.0 * Rf / 1e-4)`.

**Corrected Code Implementation:**
- **Effective Diameter ($D_{e,eff}$):** Calculated as $D_{e,clean} - 2 \cdot \delta_{shell}$, where $\delta_{shell} = R_f \cdot k_{deposit}$.
- **Effective Flow Area ($A_{s,eff}$):** Scaled from clean area based on the ratio of effective to clean diameter ($A_{s,eff} = A_{s,clean} \cdot (D_{e,eff} / D_{e,clean})$).
- **Recalculated Velocity ($v_{eff}$):** Uses the reduced effective flow area.
- **Recalculated Reynolds Number ($Re_{eff}$):** Uses $v_{eff}$ and $D_{e,eff}$.
- **Friction Factor:** Calculated using standard Blasius correlation with $Re_{eff}$ (no empirical multiplier).
- **Pressure Drop:** Computed using the updated $f$, $D_{e,eff}$, and $v_{eff}$.

---

## Changes Made

### `src/core/Hydraulics.cpp`

- **`dP_shell`**:
    - Implemented logic to calculate `De_eff` based on fouling resistance.
    - Updated velocity and Reynolds number calculations to use `De_eff` and `As_eff`.
    - Removed the empirical fouling multiplier from the friction factor calculation.
    - Updated the final pressure drop formula to use the recalculated parameters.

---

## Verification

The code now physically models the effect of fouling on the shell-side hydraulics by reducing the flow area and hydraulic diameter, which naturally increases velocity and pressure drop, consistent with the theoretical approach described in the report.
