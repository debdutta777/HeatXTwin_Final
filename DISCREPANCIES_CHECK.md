# Additional Discrepancies Check

Following the initial fixes, a further review of the codebase against the "all four.md" report has identified the following remaining discrepancies:

### 1. Critical Physics Error: Shell-Side Friction Factor
- **Report (Section 3.10.2):** States that the shell-side friction factor is obtained from "empirical relations for tube banks".
- **Code (`Hydraulics.cpp`):** Uses the **Blasius correlation** (`0.3164 * Re^-0.25`) for the shell side.
- **Impact:** Blasius is valid for smooth **internal pipe flow**, not cross-flow over tube banks. Friction factors for tube banks are typically significantly higher (often by an order of magnitude) and follow different scaling laws. Using the pipe correlation results in a gross underestimation of the shell-side pressure drop.

### 2. Simplified Viscosity Correction
- **Report (Section 3.5.2):** Mentions the Zhukauskas correlation includes a viscosity ratio term `(Pr/Pr_w)^0.25`.
- **Code (`Thermo.cpp`):** Explicitly sets this term to `1.0` with a comment: "Current approximation: Pr/Pr_w ≈ 1.0".
- **Impact:** This is a minor accuracy loss for fluids with strong viscosity-temperature dependence, but it is a deliberate simplification rather than a bug. It is noted here for completeness.

### 3. Dynamic Simulation Integration Method
- **Report (Section 3.11.3):** Mentions "fixed-step explicit method" (Forward Euler).
- **Code (`Simulator.cpp`):** Implements Explicit Euler.
- **Status:** **Consistent**. The implementation matches the report.

### 4. Fouling Model Interpretation
- **Report (Section 3.8.1):** $R_f(t) = R_{f,0} + R_{f,\infty} (1 - e^{-t/\tau})$.
- **Code (`Fouling.cpp`):** `rf = p_.Rf0 + p_.RfMax * (1.0 - exp(...))`.
- **Status:** **Consistent**. The code correctly implements the report's formula structure.

## Recommended Action
The **Shell-Side Friction Factor** error is critical and should be fixed to align with the report's "tube bank" specification. A standard correlation (e.g., Kern's or a generic tube-bank power law) should replace the Blasius equation for the shell side.
