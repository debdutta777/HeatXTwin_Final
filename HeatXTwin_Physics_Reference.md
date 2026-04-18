# HeatXTwin Pro 2.0 — Complete Physics & Engineering Reference

## Project Overview

**HeatXTwin Pro 2.0** is a real-time digital twin of a shell-and-tube heat exchanger, built in C++17 with Qt6. It couples first-principles thermal-hydraulic models with dynamic ODE/finite-volume integration, PID control with feed-forward and cascade architectures, spatial fouling distribution, TEMA flow-induced vibration checks, Monte Carlo sensitivity analysis, and live FFT spectral analysis — all visualised in an interactive desktop GUI.

The hot fluid flows inside the tubes; the cold fluid flows through the shell side across the tube bank.

---

## 1. Tube-Side Convective Heat Transfer

### 1.1 Reynolds & Prandtl Numbers

The tube-side Reynolds number is computed from the total tube-bundle flow area:

```
Re_tube = (rho_h * v * D_i) / mu_h
```

where the bulk velocity per tube is:

```
v = (m_dot_hot / rho_h) / A_cross
A_cross = (pi * D_i^2 / 4) * N_tubes
```

The Prandtl number is:

```
Pr = (c_p * mu) / k
```

| Symbol | Description | Unit |
|--------|-------------|------|
| `rho_h` | Hot-fluid density | kg/m^3 |
| `mu_h` | Hot-fluid dynamic viscosity | Pa*s |
| `D_i` | Tube inner diameter | m |
| `c_p` | Specific heat at constant pressure | J/(kg*K) |
| `k` | Thermal conductivity | W/(m*K) |
| `N_tubes` | Number of tubes | - |

### 1.2 Dittus-Boelter Correlation (Turbulent, Re >= 2300)

```
Nu = 0.023 * Re^0.8 * Pr^n       n = 0.4 (heating case)
```

The tube-side convective heat transfer coefficient is then:

```
h_tube = Nu * k_hot / D_i     [W/m^2/K]
```

**Validity**: Re > 2300, 0.6 < Pr < 160, L/D > 10.

### 1.3 Laminar Fallback (Re < 2300)

```
Nu = 4.36     (constant wall temperature assumption)
```

This is the fully-developed Nusselt number for uniform wall temperature in laminar tube flow.

**Source**: Incropera & DeWitt, *Fundamentals of Heat and Mass Transfer*.

---

## 2. Shell-Side Convective Heat Transfer

The application supports two shell-side correlations, selectable at runtime.

### 2.1 Kern Method (Simplified)

Based on the Zhukauskas correlation for cross-flow over a tube bank:

```
Nu = C * Re^m * Pr^n * (Pr/Pr_w)^0.25
```

| Constant | Value | Notes |
|----------|-------|-------|
| `C` | 0.27 | For typical staggered banks |
| `m` | 0.63 | Reynolds exponent |
| `n` | 0.37 | Prandtl exponent |
| `(Pr/Pr_w)^0.25` | ~1.0 | Approximated; valid for DeltaT < 30-40 C |

**Shell-side flow area** (Kern formula):

```
A_s = (D_s / P_t) * (P_t - D_o) * B
```

| Symbol | Description | Unit |
|--------|-------------|------|
| `D_s` | Shell inner diameter | m |
| `P_t` | Tube pitch | m |
| `D_o` | Tube outer diameter | m |
| `B` | Baffle spacing | m |

**Shell-side velocity and Reynolds number**:

```
v_shell = (m_dot_cold / rho_cold) / A_s
Re_shell = rho_cold * v_shell * D_e / mu_cold
```

**Equivalent diameter** (crude gap estimate):

```
D_e = D_s - D_o
```

**Fouling adjustment** — when fouling is present, the shell-side effective diameter shrinks:

```
delta_shell = R_f,shell * k_deposit          [m]
D_e,eff = D_e,clean - 2 * delta_shell        [m]
```

**Source**: Kern, D.Q. *Process Heat Transfer*, McGraw-Hill, 1950.

### 2.2 Bell-Delaware Method (Industry Standard)

The Bell-Delaware method (Bell, 1963) decomposes shell-side flow into ideal crossflow and applies five correction factors for real-world non-idealities.

#### 2.2.1 Bundle Geometry

```
D_otl = 0.95 * D_s                           (outer-tube-limit diameter)
D_ctl = D_otl - D_o,eff                      (centerline OTL)
```

**Crossflow area at shell centerline** (S_m):

```
S_m = B * [ (D_s - D_otl) + D_ctl * (P_t - D_o,eff) / P_eff ]
```

where `P_eff` depends on tube layout angle:

| Layout Angle | P_eff |
|--------------|-------|
| 30 deg (triangular) | P_t |
| 45 deg (rotated-square) | P_t * sqrt(2) |
| 90 deg (square) | P_t |

**Row pitch** (perpendicular to flow):

| Layout Angle | P_p |
|--------------|-----|
| 30 deg | P_t * sqrt(3)/2 |
| 45 deg | P_t / sqrt(2) |
| 90 deg | P_t |

**Tube rows crossed between baffle tips and in one window**:

```
N_c  = D_s * (1 - 2*B_c) / P_p              (crossflow region rows)
N_cw = 0.8 * B_c * D_s / P_p                (window region rows)
```

where `B_c` is the baffle cut fraction, clamped to [0.05, 0.45].

#### 2.2.2 Ideal Crossflow Coefficient (Colburn j-Factor)

The mass velocity through the crossflow area:

```
G_s = m_dot_shell / S_m       [kg/m^2/s]
```

Reynolds number (using tube OD as characteristic length):

```
Re_s = D_o,eff * G_s / mu_shell
```

**Colburn j-factor** (power-law fit, layout-angle-dependent):

| Layout | C | a |
|--------|---|---|
| 30 deg triangular | 0.321 | -0.388 |
| 45 deg rotated-sq | 0.370 | -0.396 |
| 90 deg square | 0.370 | -0.395 |

```
j = C * Re^a                  (Re >= 100)
j = 1.73 * Re^(-0.694)        (Re < 100, laminar correction)
```

The ideal crossflow heat transfer coefficient:

```
h_ideal = j * c_p * G_s / Pr^(2/3)
```

#### 2.2.3 Heat Transfer Correction Factors (J-Factors)

**J_c — Baffle-cut correction**:

```
theta_ds = 2 * acos(1 - 2*B_c)              [rad]
F_w = (theta_ds - sin(theta_ds)) / (2*pi)   (window area fraction)
F_c = 1 - 2*F_w                             (crossflow tube fraction)
J_c = 0.55 + 0.72 * F_c                     clamped [0.52, 1.15]
```

**J_l — Leakage correction**:

The leakage areas (tube-baffle gap S_tb, shell-baffle gap S_sb):

```
S_tb = pi * D_o,eff * delta_tb * N_t * (1 + F_c) / 2
S_sb = D_s * delta_sb * (pi - 0.5*theta_ds) / 2
```

| Parameter | Default Value | Notes |
|-----------|---------------|-------|
| `delta_tb` (tube-baffle gap) | 0.4 mm | TEMA Class R |
| `delta_sb` (shell-baffle gap) | 3.0 mm | Industry typical |

```
r_s  = S_sb / (S_sb + S_tb)                 (shell-to-total leakage ratio)
r_lm = (S_sb + S_tb) / S_m                  (total leakage / crossflow area)
J_l  = 0.44*(1 - r_s) + [1 - 0.44*(1 - r_s)] * exp(-2.2 * r_lm)
```

Clamped [0.2, 1.0].

**J_b — Bundle-bypass correction**:

```
S_b  = B * (D_s - D_otl)                    (bypass area)
F_bp = S_b / S_m
C_bh = 1.35 (Re < 100),  1.25 (Re >= 100)
ss_ratio = (2*N_ss / N_c)^(1/3)             (sealing strip effect)
J_b  = exp(-C_bh * F_bp * (1 - ss_ratio))   clamped [0.5, 1.0]
```

**J_s — Equal-spacing correction**: J_s = 1.0 (assumed equal baffle spacing).

**J_r — Laminar correction**:

```
Re < 20:    J_r = (10 / (N_c + N_cw))^0.18
20 < Re < 100: linear blend between laminar J_r and 1.0
Re >= 100:  J_r = 1.0
```

Clamped [0.4, 1.0].

**Final shell-side coefficient**:

```
h_shell = h_ideal * J_c * J_l * J_b * J_s * J_r
```

#### 2.2.4 Pressure Drop Correction Factors (R-Factors)

**R_l — Leakage**:

```
p = 0.8 (Re >= 100),  1.33 (Re < 100)
R_l = exp[-1.33 * (1 + r_s) * r_lm^p]      clamped [0.15, 1.0]
```

**R_b — Bypass**:

```
C_bp = 3.7 (Re >= 100),  4.5 (Re < 100)
R_b = exp[-C_bp * F_bp * (1 - ss_ratio)]    clamped [0.3, 1.0]
```

**R_s — Spacing**: R_s = 1.0 (equal spacing).

#### 2.2.5 Shell-Side Pressure Drop (Bell-Delaware)

**Ideal crossflow delta-P per baffle space**:

```
f_bank: friction factor (same angle/Re ranges as j, separate coefficients)
  30 deg:  f = 0.372 * Re^(-0.123)     (Re >= 100)
  45 deg:  f = 0.303 * Re^(-0.126)
  90 deg:  f = 0.391 * Re^(-0.148)
  Laminar: f = 45 / Re                  (Re < 100)

dP_ideal = 2 * f_bank * N_c * G_s^2 / rho_shell
```

**Window pressure drop**:

```
S_window = D_s * B_c * B * 0.5
G_w = m_dot_shell / S_window
dP_window = (2 + 0.6*N_cw) * G_w^2 / (2*rho_shell)
```

**Total shell-side pressure drop**:

```
dP_shell = (N_b - 1)*dP_ideal*R_b*R_l  +  N_b*dP_window*R_l  +  2*dP_ideal*(1 + N_cw/N_c)*R_b*R_s
```

**Sources**: Bell, K.J. (1963); Kakac & Liu, *Heat Exchangers: Selection, Rating and Thermal Design*; Serth, *Process Heat Transfer Principles and Applications*.

---

## 3. Overall Heat Transfer Coefficient

The overall coefficient is computed from the standard series-resistance network referred to the outer tube surface area:

```
1/U = 1/h_shell + R_wall + (1/h_tube)*(D_i/D_o) + R_f,shell + R_f,tube
```

**Wall resistance** (cylindrical tube):

```
R_wall = ln(D_o / D_i) * D_o / (2 * k_wall)       [m^2*K/W]
```

| Symbol | Description | Unit |
|--------|-------------|------|
| `k_wall` | Tube wall thermal conductivity | W/(m*K) |
| `D_o`, `D_i` | Tube outer/inner diameters | m |
| `R_f,shell` | Shell-side fouling resistance | m^2*K/W |
| `R_f,tube` | Tube-side fouling resistance | m^2*K/W |

The overall U [W/m^2/K]:

```
U = 1 / (1/h_shell + R_wall + (1/h_tube)*(D_i/D_o) + R_f,shell + R_f,tube)
```

**Total heat transfer area** (outer):

```
A = pi * D_o * L * N_tubes     [m^2]
```

---

## 4. Steady-State Thermal Performance (epsilon-NTU Method)

### 4.1 Capacity Rates

```
C_h   = m_dot_hot  * c_p,hot       [W/K]
C_c   = m_dot_cold * c_p,cold      [W/K]
C_min = min(C_h, C_c)
C_max = max(C_h, C_c)
C_r   = C_min / C_max              (capacity ratio, dimensionless)
```

### 4.2 Number of Transfer Units

```
NTU = (U * A) / C_min
```

### 4.3 Effectiveness by Flow Arrangement

**Counter-flow**:

```
If C_r = 1:   epsilon = NTU / (1 + NTU)
Otherwise:    epsilon = [1 - exp(-NTU*(1 - C_r))] / [1 - C_r * exp(-NTU*(1 - C_r))]
```

**Parallel-flow**:

```
epsilon = [1 - exp(-NTU*(1 + C_r))] / (1 + C_r)
```

**1-shell-pass, 2-tube-pass** (TEMA E-type):

```
s = sqrt(1 + C_r^2)
e = exp(-NTU * s)
epsilon = 2 / [(1 + C_r) + s * (1 + e)/(1 - e)]
```

**2-shell-pass, 4-tube-pass** (Kays-London recursion):

```
epsilon_1 = epsilon for 1-shell at NTU/2

If C_r = 1:   epsilon = 2*epsilon_1 / (1 + epsilon_1)
Otherwise:    E = [(1 - epsilon_1*C_r)/(1 - epsilon_1)]^2
              epsilon = (E - 1) / (E - C_r)
```

### 4.4 Heat Duty and Outlet Temperatures

```
Q     = epsilon * C_min * (T_h,in - T_c,in)        [W]
T_c,out = T_c,in + Q / C_c                          [C]
T_h,out = T_h,in - Q / C_h                          [C]
```

### 4.5 LMTD Correction Factor (Bowman, 1936)

For shell-and-tube arrangements (1-2 and 2-4 pass):

```
P = (T_c,out - T_c,in) / (T_h,in - T_c,in)
R = (T_h,in - T_h,out) / (T_c,out - T_c,in)
s = sqrt(R^2 + 1)

F = [s/(R-1)] * ln[(1-P)/(1-P*R)] / ln[(2-P*(R+1-s)) / (2-P*(R+1+s))]
```

For 2-shell-pass (2-4 arrangement), `P` is transformed:

```
ratio = (1 - P*R) / (1 - P)
P* = (1 - ratio^(1/2)) / (R - ratio^(1/2))
```

Then F is re-evaluated at (P*, R).

F is clamped to [0.3, 1.0].

**Source**: Bowman, R.A. et al. (1936), *Mean Temperature Difference in Design*, Trans. ASME, 58.

---

## 5. Tube-Side Pressure Drop

### 5.1 Friction Factor

**Laminar** (Re < 2300):

```
f = 64 / Re
```

**Turbulent** (Re >= 2300) — Blasius correlation for smooth pipes:

```
f = 0.3164 * Re^(-0.25)
```

### 5.2 Total Pressure Drop

```
dP_fric  = f * (L / D_i,eff) * 0.5 * rho * v^2      [Pa]
dP_minor = K_minor * 0.5 * rho * v^2                  [Pa]
dP_tube  = dP_fric + dP_minor
```

| Parameter | Default | Notes |
|-----------|---------|-------|
| `K_minor` | 1.5 | Entrance/exit/bend losses |

**Fouling reduces effective diameter**:

```
t_deposit = R_f,tube * k_deposit                       [m]
D_i,eff = D_i - 2 * t_deposit                         [m]
```

### 5.3 Shell-Side Pressure Drop (Kern Method)

```
f_shell = 1.44 * Re^(-0.15)
L_eq    = N_baffles * B                                (equivalent flow length)
dP_fric = f_shell * (L_eq / D_e,eff) * 0.5 * rho_cold * v_eff^2
dP_minor = K_turns * 0.5 * rho_cold * v_eff^2
dP_shell = dP_fric + dP_minor
```

With fouling, the flow area scales proportionally to the hydraulic diameter ratio:

```
A_s,eff = A_s,clean * (D_e,eff / D_e,clean)
```

(Bell-Delaware shell-side pressure drop is covered in Section 2.2.5 above.)

---

## 6. Fouling Models

### 6.1 Asymptotic Fouling (Kern-Seaton Model)

```
R_f(t) = R_f0 + R_f,max * [1 - exp(-t / tau)]      [m^2*K/W]
```

| Parameter | Symbol | Unit | Description |
|-----------|--------|------|-------------|
| Initial fouling | R_f0 | m^2*K/W | Fouling at t=0 (e.g. residual from a previous run) |
| Asymptotic maximum | R_f,max | m^2*K/W | Equilibrium between deposition and removal |
| Time constant | tau | s | Rate at which fouling approaches R_f,max |

**Physical interpretation**: Deposition rate decreases as the deposit surface temperature falls (reducing the driving force), while removal rate increases as the layer thickens (more shear), producing self-limiting growth.

### 6.2 Linear Fouling

```
R_f(t) = R_f0 + alpha * t                           [m^2*K/W]
```

| Parameter | Symbol | Unit |
|-----------|--------|------|
| Linear rate | alpha | m^2*K/(W*s) |

Used when the operating period is short relative to the asymptotic time constant or when crystallisation dominates over removal.

### 6.3 Deposit Thickness Mapping

The fouling resistance is converted to a physical deposit thickness for hydraulic-diameter reduction:

```
delta = R_f * k_deposit       [m]
```

| Parameter | Default | Unit | Notes |
|-----------|---------|------|-------|
| k_deposit | 0.5 | W/(m*K) | Effective conductivity of deposit layer |

### 6.4 Fouling Split Ratio

Total fouling resistance is distributed between shell and tube sides:

```
R_f,shell = R_f * split_ratio
R_f,tube  = R_f * (1 - split_ratio)
```

Default `split_ratio = 0.5` (equal distribution). The split controls where the deposit physically sits, affecting which hydraulic diameter shrinks and which convection coefficient is reduced.

---

## 7. Spatial Fouling Distribution (Per-Tube Fouling Map)

The bulk R_f is distributed across every tube and axial cell using a physics-motivated model that superposes four effects, then renormalises so the area-weighted mean recovers R_f,bulk exactly.

### 7.1 Radial Bypass Bias

Tubes near the shell centre see less crossflow (bypass at the periphery) and therefore lower shear, enhancing deposition:

```
f_radial(r) = 1 + A_r * (1 - r_norm^2)
```

| Constant | Value | Meaning |
|----------|-------|---------|
| A_r | 0.8 | Centre tubes foul ~80% more than wall tubes |
| r_norm | [0, 1] | Normalized radial position (0 = centre, 1 = shell wall) |

### 7.2 Baffle-Cut Stagnation

Tubes in the baffle window sit in recirculation zones that trap particulates:

```
f_window(y) = 1 + A_w * sigma(10 * (|y|/R_bundle - (1 - B_c)))
```

where sigma(x) = 1 / (1 + exp(-x)) is the logistic function.

| Constant | Value |
|----------|-------|
| A_w | 0.25 |
| B_c | Baffle cut fraction [0.15, 0.45] |

### 7.3 Axial Build-Up

Downstream cells foul progressively more:

```
f_axial(zeta) = 1 + gamma * zeta       zeta in [0, 1]
```

| Constant | Value | Meaning |
|----------|-------|---------|
| gamma | 0.6 | Downstream cells ~60% more fouled than inlet |

### 7.4 Shear Modifier (Operating Point)

Higher shell-side flow produces more shear, thinning the deposit:

```
shear = sqrt(2.0 / m_dot_cold)         normalised at m_dot_cold = 2 kg/s
```

Clamped to [0.5, 2.0].

### 7.5 Per-Cell Fouling Resistance

```
R_f(i, j)_raw = R_f,bulk * f_radial(i) * f_window(i) * shear * f_axial(j)
```

### 7.6 Renormalisation (Mass Conservation)

After computing raw values for all tubes and cells:

```
raw_mean = sum(R_f_mean_tube(i) * A_tube) / sum(A_tube)
scale = R_f,bulk / raw_mean
R_f(i, j) = R_f(i, j)_raw * scale
```

This ensures the total deposit mass (proportional to area-weighted mean R_f) is conserved.

### 7.7 Hot-Spot Detection

A tube is flagged as a "hot spot" when its axial-mean R_f exceeds 1.5 times the bulk value:

```
hot_spot_fraction = count(R_f_mean(i) > 1.5 * R_f,bulk) / N_tubes
```

### 7.8 Tube Layout Generation

Tubes are placed on an equilateral-triangular pitch grid inside a circle of radius R_bundle = 0.5*D_s - 0.5*D_o. Candidates are sorted by distance from centre and the closest N_tubes are selected, replicating real bundle packing.

---

## 8. Dynamic Simulation

### 8.1 Lumped-Parameter Model (Two ODEs)

The dynamic model uses thermal holdup masses M_h and M_c [kg] to represent the energy storage in each fluid volume:

**Hot-side energy balance**:

```
M_h * c_p,h * dT_h,out/dt = m_dot_h * c_p,h * (T_h,in - T_h,out) - Q
```

**Cold-side energy balance**:

```
M_c * c_p,c * dT_c,out/dt = m_dot_c * c_p,c * (T_c,in - T_c,out) + Q
```

where:

```
Q = U * A * (T_h,out - T_c,out) * F
```

F is the LMTD correction factor (Section 4.5).

**Time integration**: Explicit (forward) Euler:

```
T_out(t + dt) = T_out(t) + (dT_out/dt) * dt
```

**Numerical safeguards**:
- Temperature clipping: T in [0, 200] C
- Heat duty clipping: Q in [0, 1 MW]
- Division guards: max(denominator, 1e-12)

### 8.2 Finite-Volume Axial Discretization (N-Cell Model)

When `numAxialCells > 1`, the exchanger is sliced into N streamwise cells. Each cell has its own T_h and T_c, advancing via upwind advection + a local heat source term.

**Per-cell heat transfer**:

```
q_i = U * dA * (T_h,i - T_c,i) * F
dA = A_total / N
```

**Hot-side advection** (flows from cell 0 to N-1):

```
dT_h,i/dt = [C_h * (T_h,upstream - T_h,i) - q_i] / C_th,h,cell
```

**Cold-side advection** (direction depends on flow arrangement):
- Counter-flow: cold enters at cell N-1, exits at cell 0
- Parallel-flow: cold enters at cell 0, exits at cell N-1

```
dT_c,i/dt = [C_c * (T_c,upstream - T_c,i) + q_i] / C_th,c,cell
```

**Cell thermal capacitance**:

```
C_th,h,cell = (M_h / N) * c_p,h
C_th,c,cell = (M_c / N) * c_p,c
```

**CFL-type sub-stepping**: The macro time step dt is subdivided so the fluid advances at most half a cell per sub-step:

```
tau_h = C_th,h,cell / C_h       (hot-side cell residence time)
tau_c = C_th,c,cell / C_c       (cold-side cell residence time)
tau_min = min(tau_h, tau_c)
n_sub = ceil(dt / (0.5 * tau_min))
dt_sub = dt / n_sub
```

**Outlet temperatures**:

```
T_h,out = T_h[N-1]                          (always at the hot-outlet end)
T_c,out = T_c[0]       (counter-flow)
        = T_c[N-1]     (parallel-flow)
```

### 8.3 Temperature-Dependent Properties

When a fluid preset is selected (not "Custom"), the simulator re-evaluates rho, mu, c_p, k at the mean film temperature on every time step:

```
T_mean = 0.5 * (T_in + T_out)
```

The updated properties are fed back into both the Thermo and Hydraulics calculations, creating a closed feedback loop between transport properties and flow/heat-transfer.

### 8.4 Disturbance Injection

Three disturbance modes simulate real-world process upsets:

**Sine wave** (multi-harmonic):

```
T_h,in += A * sin(2*pi*t / P) + 0.5*A * sin(2*pi*t / (P/3))
T_c,in += 0.66*A * sin(2*pi*t / (1.33*P)) + 0.33*A * sin(2*pi*t / (0.5*P))
m_dot_h += m_dot_h * f * sin(2*pi*t / P_flow) + m_dot_h * 0.5*f * sin(2*pi*t / (0.4*P_flow))
m_dot_c += m_dot_c * 0.8*f * sin(2*pi*t / (0.73*P_flow)) + m_dot_c * 0.4*f * sin(2*pi*t / (0.26*P_flow))
```

| Default | Symbol | Value | Unit |
|---------|--------|-------|------|
| Temp amplitude | A | 3.0 | C |
| Temp period | P | 900 | s |
| Flow amplitude | f | 0.15 | fraction |
| Flow period | P_flow | 1500 | s |

**Step change**: at time t_step, instantly shift flow by a fraction and inlet temperature by delta-T.

**Ramp**: linearly ramp flow rate over a specified duration starting at t_start.

### 8.5 Scenario Engine

A list of `ScenarioEvent` objects provides a scripted timeline. Each event fires once when its trigger time is reached:

| Action | Effect |
|--------|--------|
| SetHotFlow | Set m_dot_hot to a new value |
| SetColdFlow | Set m_dot_cold to a new value |
| SetHotInletT | Set T_h,in to a new value |
| SetColdInletT | Set T_c,in to a new value |
| SetPidSetpoint | Change the PID setpoint |
| SetPidEnabled | Enable or disable PID control |
| FoulingJump | Add an instantaneous delta to R_f (cleaning = negative) |
| Mark | Annotation only — no plant effect |

---

## 9. PID Control System

### 9.1 Standard Three-Term PID

The PID controller regulates cold-side mass flow (manipulated variable, MV) to track a desired cold outlet temperature (process variable, PV):

```
e(t) = SP - T_c,out(t)                          [K]
```

**Proportional term**:

```
u_p = K_p * e
```

**Integral term** (with anti-windup):

```
u_i += K_i * e * dt
u_i = clamp(u_i, u_min, u_max)
```

**Derivative term** (backward difference):

```
u_d = K_d * (e - e_prev) / dt
```

**Output** (with rate limiting):

```
u_new = u_p + u_i + u_d
du = clamp(u_new - u_prev, -rate_limit*dt, +rate_limit*dt)
u = clamp(u_prev + du, u_min, u_max)
```

| Parameter | Default | Unit | Description |
|-----------|---------|------|-------------|
| K_p | 0.05 | kg/s/K | Proportional gain |
| K_i | 0.005 | kg/s/K/s | Integral gain |
| K_d | 0.0 | kg*s/K | Derivative gain |
| u_min | 0.1 | kg/s | Minimum cold flow |
| u_max | 5.0 | kg/s | Maximum cold flow |
| rate_limit | 0.5 | kg/s/s | Max rate of change |
| SP | 45.0 | C | Setpoint temperature |

### 9.2 Feed-Forward Control

Two measurable disturbances are compensated before they affect the PV:

```
u_FF = k_FF,Tin * (T_in,hot - T_in,hot,nom) + k_FF,flow * (m_dot_hot - m_dot_hot,nom)
```

**Auto-derived gains** (from the steady-state energy balance):

Starting from Q = m_h * c_p,h * (T_in,h - T_h,out) = m_c * c_p,c * (SP - T_in,c):

```
k_FF,Tin  = (m_dot_h * c_p,h) / (c_p,c * (SP - T_in,c))
k_FF,flow = c_p,h * (T_in,h - T_h,out) / (c_p,c * (SP - T_in,c))
```

These are the partial derivatives of the required cold flow with respect to each disturbance, evaluated at the nominal operating point.

### 9.3 Cascade / Actuator Lag

A first-order lag models the valve dynamics between the PID output (commanded flow) and the actual flow reaching the exchanger:

```
dm_actual/dt = (m_cmd - m_actual) / tau_valve
```

Discretised with explicit Euler:

```
m_actual += dt * (m_cmd - m_actual) / max(tau_valve, dt)
```

The exchanger sees `m_actual`, not `m_cmd`, introducing transport delay into the control loop and requiring the controller to be tuned for the slower effective dynamics.

---

## 10. Relay Autotune (Astrom-Hagglund Method)

### 10.1 Relay Experiment

The cold flow is driven by a bang-bang relay that alternates between two values:

```
m_dot_cold = mv_bias + h      (relay ON)
m_dot_cold = mv_bias - h      (relay OFF)
```

Switching occurs when T_c,out crosses the setpoint with hysteresis:

```
Switch OFF when T_c,out < SP - hysteresis
Switch ON  when T_c,out > SP + hysteresis
```

| Setting | Default | Unit |
|---------|---------|------|
| relay_h | 0.5 | kg/s |
| mv_bias | 1.0 | kg/s |
| hysteresis | 0.1 | C |
| requiredCycles | 5 | - |
| maxSimTime | 2000 | s |

### 10.2 Ultimate Parameter Extraction

From the sustained oscillation:

```
a = (T_max - T_min) / 2                      (half-amplitude) [K]
K_u = 4*h / (pi * a)                         (ultimate gain) [kg/s per K]
P_u = average period between upward crossings [s]
```

### 10.3 Ziegler-Nichols "Classic PID" Tuning Rule

```
K_p = 0.6 * K_u
T_i = 0.5 * P_u      -->  K_i = K_p / T_i
T_d = 0.125 * P_u    -->  K_d = K_p * T_d
```

**Source**: Astrom, K.J. & Hagglund, T. (1984), "Automatic Tuning of Simple Regulators with Specifications on Phase and Amplitude Margins", *Automatica*, 20(5).

---

## 11. TEMA Flow-Induced Vibration (FIV) Analysis

### 11.1 Natural Frequency (1st Bending Mode, Euler-Bernoulli Beam)

```
f_n = (C / (2*pi*L_eff^2)) * sqrt(E*I / m_e)      [Hz]
```

**End-condition coefficient C**:

| Support | C | Description |
|---------|---|-------------|
| Pinned-pinned | 9.870 (pi^2) | Simply supported both ends |
| Clamped-clamped | 22.373 | Rigidly fixed both ends |
| Clamped-pinned | 15.418 | One fixed, one simply supported |

**Second moment of area** (hollow tube):

```
I = (pi/64) * (D_o^4 - D_i^4)                       [m^4]
```

**Flexural rigidity**:

```
EI = E * I                                           [N*m^2]
```

**Effective mass per unit length** (three contributions):

```
m_e = rho_tube * A_metal + rho_tube_fluid * A_inside + C_m * rho_shell * A_outside
```

where:

```
A_metal   = (pi/4) * (D_o^2 - D_i^2)     (tube wall cross-section)
A_inside  = (pi/4) * D_i^2                (contained tube-side fluid)
A_outside = (pi/4) * D_o^2                (displaced shell-side fluid)
```

| Parameter | Default | Unit | Notes |
|-----------|---------|------|-------|
| E (Young's modulus) | 193 GPa | Pa | SS 304/316 |
| rho_tube | 8000 | kg/m^3 | Stainless steel |
| C_m (added mass coeff) | 1.5 | - | For liquid shell-side service |
| L_eff | B (baffle spacing) | m | Unsupported span |

### 11.2 Vortex-Shedding Excitation

```
f_v = St * V_cross / D_o                             [Hz]
```

| Parameter | Default |
|-----------|---------|
| Strouhal number (St) | 0.2 |

**Crossflow velocity**:

```
S_m = (D_s/P_t) * (P_t - D_o) * B
V_cross = (m_dot_shell / rho_shell) / S_m            [m/s]
```

**Lock-in criterion**:

| f_v / f_n | Status |
|-----------|--------|
| < 0.5 | SAFE |
| 0.5 - 0.8 | MARGINAL |
| >= 0.8 | FAIL |

### 11.3 Fluid-Elastic Instability (Connors Criterion)

The critical velocity above which self-excited vibration begins:

```
V_crit = K * f_n * D_o * sqrt(m_e * delta / (rho_shell * D_o^2))     [m/s]
```

| Parameter | Default |
|-----------|---------|
| Connors K | 3.3 (liquid cross-flow) |
| Logarithmic decrement (delta) | 0.03 (water-in-shell) |

**Stability criterion**:

| V_cross / V_crit | Status |
|-------------------|--------|
| < 0.5 | SAFE |
| 0.5 - 1.0 | MARGINAL |
| >= 1.0 | FAIL |

### 11.4 Acoustic Resonance

**Speed of sound** (auto-detected from shell-fluid density):

```
c = 1500 m/s    if rho_shell > 100 kg/m^3  (liquid)
c = 340  m/s    if rho_shell <= 100 kg/m^3  (gas)
```

**1st transverse acoustic mode of the shell**:

```
f_a = c / (2 * D_s)                                  [Hz]
```

**Safety margin**:

```
margin = |f_a - f_v| / max(f_a, f_v)
```

| Margin | Status |
|--------|--------|
| > 0.20 | SAFE |
| 0.05 - 0.20 | MARGINAL |
| < 0.05 | FAIL |

The overall vibration verdict is the worst of the three individual checks.

**Source**: TEMA Standards, Section 6 (Flow-Induced Vibration); Blevins, *Flow-Induced Vibration*, Van Nostrand.

---

## 12. Monte Carlo Sensitivity Analysis

### 12.1 Perturbation Scheme

Each trial perturbs the plant inputs independently with Gaussian noise:

**Fractional perturbations** (sigma * nominal):

| Parameter | Default sigma | Description |
|-----------|---------------|-------------|
| m_dot_hot | 5% | Hot flow uncertainty |
| m_dot_cold | 5% | Cold flow uncertainty |
| rho (both fluids) | 2% | Density uncertainty |
| mu (both fluids) | 10% | Viscosity uncertainty |
| c_p (both fluids) | 2% | Specific heat uncertainty |
| k (both fluids) | 5% | Conductivity uncertainty |
| R_f,max | 15% | Fouling model uncertainty |

**Absolute perturbations**:

| Parameter | Default sigma | Unit |
|-----------|---------------|------|
| T_in,hot | 1.0 | K |
| T_in,cold | 0.5 | K |

Each perturbed parameter: `x_trial = x_nominal * max(0.1, 1.0 + sigma_frac * N(0,1))`

Default ensemble: 200 trials, seed = 42.

### 12.2 Trial Execution

Each trial runs a short dynamic simulation (60 s, dt = 0.5 s) in lumped mode (single cell, no PID, no disturbances) and records the final-state KPIs:

- Q [W], U [W/m^2*K], T_c,out [C], T_h,out [C]
- dP_tube [Pa], dP_shell [Pa]
- epsilon = Q / Q_max (effectiveness)
- NTU = U*A / C_min

### 12.3 Statistical Output

For each KPI: mean, standard deviation, p5, p50 (median), p95, min, max.

### 12.4 Tornado Analysis

For each input parameter, two additional trials are run at +1 sigma and -1 sigma (all others at nominal):

```
delta_plus  = Q(baseline + sigma) - Q(baseline)
delta_minus = Q(baseline - sigma) - Q(baseline)
sensitivity = max(|delta_plus|, |delta_minus|)
```

Results are sorted by decreasing sensitivity to identify the dominant uncertainty sources.

---

## 13. Live FFT Spectral Analysis

### 13.1 Data Acquisition

A ring buffer of 512 samples of T_c,out is maintained. The sampling interval is estimated from successive arrival times:

```
dt_est = t_current - t_previous     (updated on each sample)
f_s = 1 / dt_est                    (sampling rate) [Hz]
```

### 13.2 Windowing

A Hann window suppresses spectral leakage:

```
w[i] = 0.5 * (1 - cos(2*pi*i / (N-1)))     for i = 0, ..., N-1
```

The DC component is removed by subtracting the mean before windowing:

```
x[i] = (T_c,out[i] - mean) * w[i]
```

### 13.3 FFT Algorithm

An in-place iterative radix-2 Cooley-Tukey FFT (N = 512, must be power of 2):

1. **Bit-reverse permutation** of the input array
2. **Butterfly stages**: for each stage length L = 2, 4, ..., N:

```
twiddle = exp(-j * 2*pi / L)
For each butterfly pair (i, i + L/2):
    u = x[i]
    v = x[i + L/2] * w
    x[i]       = u + v
    x[i + L/2] = u - v
    w *= twiddle
```

### 13.4 Spectrum Display

- Magnitude: `|X(k)| * 2/N` for each frequency bin k
- Frequency resolution: `df = f_s / N`
- Only the first half (N/2 bins) is shown (real-valued input is symmetric)
- Y-axis: logarithmic scale (base 10)
- Peak detection: the bin with maximum magnitude (excluding DC) is marked with a dashed red line
- Period readout: `P_u = 1 / f_peak`

The spectrum is recomputed every 16 incoming samples after the buffer is at least 50% full. The chart is updated using a single `replace()` call (not 256 individual `append()` calls) to avoid flooding the Qt event queue.

---

## 14. Fluid Property Correlations

All correlations evaluate density, viscosity, specific heat, and thermal conductivity as functions of temperature T [C].

### 14.1 Water (IAPWS-IF97 polynomial fits, 5-150 C)

```
rho = 999.9 + 0.05*T - 0.0074*T^2                     [kg/m^3]
mu  = 2.414e-5 * 10^(247.8 / (T_K - 140))              [Pa*s]     (Vogel form)
c_p = 4217 - 3.17*T + 0.089*T^2 - 0.00068*T^3          [J/kg/K]
k   = 0.5706 + 0.001756*T - 6.46e-6*T^2                 [W/m/K]
```

### 14.2 Ethylene Glycol 30% / Water (ASHRAE HF 2017, -10 to 90 C)

```
rho = 1043.6 - 0.38*T - 0.0025*T^2                     [kg/m^3]
mu  = 0.00386 * exp(-0.024*T)                           [Pa*s]
c_p = 3727 + 3.5*T                                      [J/kg/K]
k   = 0.458 + 0.0011*T                                  [W/m/K]
```

### 14.3 Ethylene Glycol 50% / Water (ASHRAE HF 2017, -30 to 90 C)

```
rho = 1077 - 0.43*T - 0.0018*T^2                       [kg/m^3]
mu  = 0.00772 * exp(-0.0285*T)                          [Pa*s]
c_p = 3300 + 3.9*T                                      [J/kg/K]
k   = 0.381 + 0.0010*T                                  [W/m/K]
```

### 14.4 SAE 30 Engine Oil (Incropera Table A.5, 20-150 C)

```
rho = 889 - 0.62*T                                     [kg/m^3]
mu  = 0.1 * 10^(4.2 - 0.019*T)                         [Pa*s]     clamped [0.002, 0.8]
c_p = 1880 + 3.7*T                                      [J/kg/K]
k   = 0.145 - 1.3e-4*T                                  [W/m/K]
```

### 14.5 Dry Air at 1 atm (0-200 C)

```
rho = 353 / T_K                                        [kg/m^3]   (ideal gas)
mu  = 1.458e-6 * T_K^1.5 / (T_K + 110.4)              [Pa*s]     (Sutherland's law)
c_p = 1005 + 0.08*T                                     [J/kg/K]
k   = 0.0241 + 7.5e-5*T                                 [W/m/K]
```

(T_K = T + 273.15 for all correlations using absolute temperature.)

---

## 15. Physical Constants & Default Parameters

### 15.1 Mathematical Constants

| Constant | Value |
|----------|-------|
| pi | 3.14159265358979323846 |
| sqrt(2) | 1.4142... |
| sqrt(3)/2 | 0.8660... |

### 15.2 Material & Structural Defaults

| Parameter | Value | Unit | Source/Notes |
|-----------|-------|------|-------------|
| Young's modulus (SS 304/316) | 193 x 10^9 | Pa | ASME B&PV Code |
| Tube material density (SS) | 8000 | kg/m^3 | Standard handbooks |
| Added mass coefficient C_m | 1.5 | - | Liquid-side service |
| Damping log decrement delta | 0.03 | - | Water-in-shell typical |
| Strouhal number St | 0.2 | - | Equilateral tube banks |
| Connors constant K | 3.3 | - | TEMA, liquid cross-flow |
| Speed of sound (liquid) | 1500 | m/s | Water-like |
| Speed of sound (gas) | 340 | m/s | Air-like |

### 15.3 Fouling & Hydraulic Defaults

| Parameter | Value | Unit |
|-----------|-------|------|
| Deposit conductivity k_dep | 0.5 | W/(m*K) |
| Fouling split ratio | 0.5 | - |
| Tube minor loss K_minor | 1.5 | - |
| Tube-baffle gap (TEMA R) | 0.4 | mm |
| Shell-baffle gap | 3.0 | mm |
| D_otl / D_s ratio | 0.95 | - |
| Default layout angle | 30 | deg |

### 15.4 Simulation Defaults

| Parameter | Default | Unit |
|-----------|---------|------|
| Axial cells | 20 | - |
| FFT window | 512 | samples |
| FFT recompute interval | 16 | samples |
| UI emit cap | 50 Hz (20 ms gap) | - |

---

## 16. Key Source Files

| File | Purpose | Core Equations |
|------|---------|----------------|
| `Thermo.cpp` | Tube/shell h, overall U, epsilon-NTU, steady state | Dittus-Boelter, Zhukauskas, Bowman F |
| `BellDelaware.cpp` | Shell-side h and dP (BD method) | Colburn j, J/R correction factors |
| `Hydraulics.cpp` | Tube and shell pressure drops | Darcy-Weisbach, Blasius, Kern f |
| `Fouling.cpp` | Time-dependent R_f | Asymptotic / Linear models |
| `FoulingMap.cpp` | Spatial fouling distribution | Radial + axial + baffle-cut + shear |
| `Simulator.cpp` | Dynamic ODE/FV integration + control | Lumped & N-cell models, PID/FF/cascade |
| `ControllerPID.cpp` | Discrete PID with anti-windup | K_p*e + integral + derivative |
| `AutoTune.cpp` | Relay autotune experiment | Astrom-Hagglund + Ziegler-Nichols |
| `VibrationCheck.cpp` | TEMA FIV diagnostics | Euler-Bernoulli, Connors, acoustic |
| `MonteCarlo.cpp` | Sensitivity analysis | MC ensemble + tornado chart |
| `FluidLibrary.cpp` | T-dependent fluid properties | IAPWS, ASHRAE, Incropera, Sutherland |
| `Scenario.cpp` | Scripted timeline events | Deferred mutations to operating point |
| `Types.hpp` | All data structures | Geometry, Fluid, State, FoulingParams |

---

## 17. Model Assumptions & Boundary Conditions

1. **Steady-state initialisation**: The simulator starts from a steady-state energy balance (epsilon-NTU) and applies a small +/- 0.1 C perturbation to seed dynamics.

2. **Temperature-dependent properties**: When a preset fluid is selected, rho/mu/c_p/k are re-evaluated at the mean film temperature on every time step, creating a self-consistent feedback loop.

3. **Fouling distribution**: Shell-side and tube-side fouling resistances are derived from a single bulk R_f via a configurable split ratio. The spatial map renormalises to conserve total fouling mass.

4. **Flow arrangement**: The LMTD correction factor F is computed from the Bowman correlation for multi-pass configurations (1-2 and 2-4); for pure counter-flow and parallel-flow, F = 1 by definition.

5. **Axial discretization**: The finite-volume model uses first-order upwind advection with CFL-limited sub-stepping. This is stable and non-oscillatory but introduces numerical diffusion proportional to the cell size.

6. **PID anti-windup**: The integral term is clamped independently to [u_min, u_max] before summing with P and D terms. The output is additionally rate-limited to prevent actuator damage.

7. **Autotune**: Runs on a separate, self-contained simulator instance with fouling and disturbances disabled, so identification captures the clean plant dynamics without contamination from the live simulation.

8. **Vibration check**: Uses baffle spacing as the unsupported span (conservative for standard segmental baffles). The three TEMA checks (vortex-shedding, fluid-elastic instability, acoustic resonance) are evaluated independently and the worst governs.

9. **Monte Carlo**: Each trial runs a short dynamic simulation (not just a steady-state snapshot), capturing transient settling effects. PID and disturbances are disabled to isolate parametric sensitivity from control dynamics.

10. **Numerical stability**: All divisions are guarded by max(denominator, 1e-9 or 1e-12). Temperatures are clipped to [0, 200] C. Heat duty is clipped to [0, 1 MW]. R_f is clipped to [0, 0.01] m^2*K/W.

---

## 18. References

1. Kern, D.Q. (1950). *Process Heat Transfer*. McGraw-Hill.
2. Bell, K.J. (1963). "Final Report of the Cooperative Research Program on Shell and Tube Heat Exchangers." University of Delaware, Engineering Experimental Station Bulletin No. 5.
3. Kakac, S. & Liu, H. (2002). *Heat Exchangers: Selection, Rating and Thermal Design*, 2nd ed. CRC Press.
4. Serth, R.W. (2007). *Process Heat Transfer: Principles and Applications*. Academic Press.
5. Incropera, F.P. & DeWitt, D.P. (2007). *Fundamentals of Heat and Mass Transfer*, 6th ed. Wiley.
6. Bowman, R.A., Mueller, A.C. & Nagle, W.M. (1936). "Mean Temperature Difference in Design." *Trans. ASME*, 58, 283-294.
7. TEMA (Tubular Exchanger Manufacturers Association). *Standards of the Tubular Exchanger Manufacturers Association*, 10th ed.
8. Blevins, R.D. (1990). *Flow-Induced Vibration*, 2nd ed. Van Nostrand Reinhold.
9. Astrom, K.J. & Hagglund, T. (1984). "Automatic Tuning of Simple Regulators with Specifications on Phase and Amplitude Margins." *Automatica*, 20(5), 645-651.
10. Ziegler, J.G. & Nichols, N.B. (1942). "Optimum Settings for Automatic Controllers." *Trans. ASME*, 64, 759-768.
11. IAPWS-IF97 (International Association for the Properties of Water and Steam).
12. ASHRAE (2017). *ASHRAE Handbook — Fundamentals*, Chapter 31: Physical Properties of Secondary Coolants.
13. Cooley, J.W. & Tukey, J.W. (1965). "An Algorithm for the Machine Calculation of Complex Fourier Series." *Mathematics of Computation*, 19(90), 297-301.

---

*Document generated from source code analysis of HeatXTwin Pro 2.0. All formulas, constants, and ranges reflect the actual implementation.*
