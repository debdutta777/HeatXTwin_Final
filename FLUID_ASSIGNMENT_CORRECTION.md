# Fluid Assignment Correction - Alignment with Report

**Date:** November 24, 2025  
**Status:** ✅ COMPLETED

---

## Overview

The code has been updated to align with the report's specification regarding fluid placement.

**Report Specification (Section 3.4, 6.1.1):**
- **Tube Side:** Hot Fluid ($Re_h$)
- **Shell Side:** Cold Fluid ($Re_c$)

**Previous Code Implementation:**
- Tube Side: Cold Fluid
- Shell Side: Hot Fluid

**Corrected Code Implementation:**
- **Tube Side:** Hot Fluid (using `hot_` properties)
- **Shell Side:** Cold Fluid (using `cold_` properties)

---

## Changes Made

### 1. `src/core/Thermo.cpp` & `src/core/Thermo.hpp`

- **`h_tube`**:
    - Now accepts `m_dot_hot`.
    - Uses `hot_` fluid properties (density, viscosity, conductivity, Pr).
    - **Dittus-Boelter Exponent:** Since the hot fluid in the tubes is being **cooled** by the shell-side cold fluid, the exponent `n` is set to **0.3** (cooling).
- **`h_shell`**:
    - Now accepts `m_dot_cold`.
    - Uses `cold_` fluid properties.
    - Applies Zhukauskas correlation to the cold fluid.
- **`h_shell_with_fouling`**:
    - Now accepts `m_dot_cold`.
    - Uses `cold_` fluid properties.
- **`U`**:
    - Updated to pass `m_dot_hot` to `h_tube` and `m_dot_cold` to `h_shell`.

### 2. `src/core/Hydraulics.cpp` & `src/core/Hydraulics.hpp`

- **`dP_tube`**:
    - Now accepts `m_dot_hot`.
    - Uses `hot_` fluid properties for pressure drop calculation.
- **`dP_shell`**:
    - Now accepts `m_dot_cold`.
    - Uses `cold_` fluid properties for pressure drop calculation.

### 3. `src/core/Simulator.cpp`

- **`step`**:
    - Updated `hydro_.dP_tube` call to pass `dynamic_op.m_dot_hot`.
    - Updated `hydro_.dP_shell` call to pass `dynamic_op.m_dot_cold`.

---

## Verification

The code logic now consistently places the hot fluid in the tubes and the cold fluid in the shell, matching the documentation and report preferences.
