#pragma once

#include "Types.hpp"

namespace hx {

/**
 * \brief Tunable assumptions for the Bell–Delaware method.
 *
 *  The BD method depends on bundle clearances that aren't usually captured
 *  in top-level geometry inputs.  We expose them here so the UI (or the
 *  report) can tweak them, but the defaults are industry-typical values.
 */
struct BellDelawareConfig {
  /// Outer-tube-limit diameter as a fraction of shell ID (D_otl = ratio · Ds)
  double D_otl_ratio       = 0.95;
  /// Clearance between tube OD and baffle-hole (δtb) [m]
  double tube_baffle_gap   = 0.0004;  // 0.4 mm (TEMA class R)
  /// Clearance between shell ID and baffle OD (δsb) [m]
  double shell_baffle_gap  = 0.003;   // 3 mm
  /// Number of sealing-strip pairs in the bundle-shell bypass gap
  int    n_sealing_strips  = 0;
  /// Tube pitch-layout angle (30 = equilateral triangular, 45 = rotated-square, 90 = square)
  double layout_angle_deg  = 30.0;
};

/**
 * \brief Full Bell–Delaware shell-side result (h_s and ΔP_s plus diagnostics).
 *
 *  The five J-factors (J_c, J_l, J_b, J_s, J_r) multiply the ideal
 *  crossflow h to produce h_shell.  The three R-factors (R_b, R_l, R_s)
 *  multiply the ideal crossflow ΔP contributions to produce ΔP_shell.
 *  Exposing them is useful for textbook-style diagnostics in the report.
 */
struct BellDelawareResult {
  double h_shell  = 0.0;   // [W/m²K]  — overall shell-side coefficient
  double dP_shell = 0.0;   // [Pa]     — overall shell-side pressure drop
  double Re_s     = 0.0;   // shell-side Re (based on Sm and Do)
  double Sm       = 0.0;   // [m²] crossflow area at centerline
  double Nc       = 0.0;   // tube rows crossed between baffle tips
  double Ncw      = 0.0;   // tube rows crossed in one window
  double h_ideal  = 0.0;   // ideal crossflow h (before J corrections)
  double dP_ideal = 0.0;   // ideal-crossflow per-baffle ΔP (before R corrections)
  // Heat-transfer correction factors:
  double Jc = 1.0, Jl = 1.0, Jb = 1.0, Js = 1.0, Jr = 1.0;
  // Pressure-drop correction factors:
  double Rb = 1.0, Rl = 1.0, Rs = 1.0;
};

/**
 * \brief Compute Bell–Delaware h and ΔP for the shell side.
 *
 *  Implements (in abridged form) the method of Bell (1963) as presented by
 *  Kakaç & Liu, *Heat Exchangers: Selection, Rating and Thermal Design*,
 *  and Serth, *Process Heat Transfer Principles and Applications*.
 *
 *  \param g             Heat-exchanger geometry.
 *  \param shell_fluid   Shell-side fluid properties (evaluated at bulk T).
 *  \param m_dot_shell   Shell-side mass flow [kg/s].
 *  \param Rf_shell      Shell-side fouling resistance [m²K/W] (thickens the
 *                       tube ODs and reduces Sm; set 0 for clean).
 *  \param k_deposit     Deposit thermal conductivity [W/m·K].
 *  \param cfg           Bundle-clearance assumptions (defaults are typical).
 */
BellDelawareResult computeBellDelaware(
    const Geometry   &g,
    const Fluid      &shell_fluid,
    double            m_dot_shell,
    double            Rf_shell    = 0.0,
    double            k_deposit   = 0.5,
    const BellDelawareConfig &cfg = {});

} // namespace hx
