#include "BellDelaware.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

namespace {

constexpr double PI = 3.14159265358979323846;

// sqrt(2) and sqrt(3)/2 show up in the layout-angle geometry below.
// Hoisted once so every Bell-Delaware evaluation avoids two sqrt() calls.
const double SQRT2       = std::sqrt(2.0);
const double SQRT3_OVER2 = std::sqrt(3.0) / 2.0;

/** Colburn j-factor for crossflow over an ideal tube bank.
 *  Coefficients from Kakaç/Bell tables for the most common layout angle.
 *  For educational use we blend the three common layouts into one smooth
 *  power-law with small angle-dependent tweaks.
 */
double colburn_j(double Re, double angle_deg) {
  Re = std::max(Re, 1.0);
  // Base: 30° triangular  j = 0.321 · Re⁻⁰·³⁸⁸  (Re > 10³)
  double a1 = 0.321;
  double a2 = -0.388;
  if (angle_deg < 35.0) {                         // 30° triangular
      a1 = 0.321; a2 = -0.388;
  } else if (angle_deg < 60.0) {                   // 45° rotated-square
      a1 = 0.370; a2 = -0.396;
  } else {                                          // 90° square
      a1 = 0.370; a2 = -0.395;
  }
  // Low-Re laminar correction: j = 1.73 · Re⁻⁰·⁶⁹⁴ (Re < 100)
  if (Re < 100.0) return 1.73 * std::pow(Re, -0.694);
  return a1 * std::pow(Re, a2);
}

/** Fanning-style friction factor for an ideal tube bank (crossflow). */
double friction_f_bank(double Re, double angle_deg) {
  Re = std::max(Re, 1.0);
  // 30° triangular:  f = 0.372 · Re⁻⁰·¹²³  (Re > 10³)
  double b1 = 0.372;
  double b2 = -0.123;
  if (angle_deg < 35.0)       { b1 = 0.372; b2 = -0.123; }
  else if (angle_deg < 60.0)  { b1 = 0.303; b2 = -0.126; }
  else                         { b1 = 0.391; b2 = -0.148; }
  if (Re < 100.0) return 45.0 / Re;                    // laminar envelope
  return b1 * std::pow(Re, b2);
}

} // anonymous namespace

BellDelawareResult computeBellDelaware(const Geometry &g,
                                       const Fluid    &shell_fluid,
                                       double          m_dot_shell,
                                       double          Rf_shell,
                                       double          k_deposit,
                                       const BellDelawareConfig &cfg) {
  BellDelawareResult r{};

  const double Ds  = std::max(1e-6, g.shellID);
  const double Do  = std::max(1e-6, g.Do);
  const double Pt  = std::max(1e-6, g.pitch);
  const double B   = std::max(1e-6, g.baffleSpacing);
  const int    Nb  = std::max(1, g.nBaffles);
  const double mu  = std::max(1e-9, shell_fluid.mu);

  // --- Fouling thickens the OD seen by the shell fluid ---------------------
  const double delta = std::max(0.0, Rf_shell) * std::max(1e-6, k_deposit);
  const double Do_eff = std::max(1e-6, Do + 2.0 * delta);

  // --- Bundle geometry -----------------------------------------------------
  const double D_otl = std::clamp(cfg.D_otl_ratio, 0.5, 0.999) * Ds;
  const double D_ctl = std::max(1e-6, D_otl - Do_eff);  // centerline-to-centerline OTL

  // --- Crossflow area at shell centerline (Sm) -----------------------------
  //     Sm = B · [ (Ds − D_otl) + (D_ctl · (Pt − Do_eff) / P_eff) ]
  // P_eff depends on layout:  30° → Pt, 45° → Pt·√2, 90° → Pt
  const double angle = cfg.layout_angle_deg;
  double P_eff = Pt;
  if (angle > 35.0 && angle < 60.0) P_eff = Pt * SQRT2;
  const double bypassGap = std::max(0.0, Ds - D_otl);
  const double Sm = B * ( bypassGap + D_ctl * (Pt - Do_eff) / std::max(P_eff, 1e-9) );
  r.Sm = std::max(Sm, 1e-9);

  // --- Shell-side mass velocity and Reynolds -------------------------------
  const double Gs = m_dot_shell / r.Sm;                              // [kg/m²·s]
  const double Re = std::max(1.0, Do_eff * Gs / mu);                 // Bell uses Do
  r.Re_s = Re;

  // --- Tube rows crossed between baffle tips (Nc) and per window (Ncw) ----
  //  Row-pitch along crossflow:
  //     30°: Pp = Pt · √3/2,   45°: Pp = Pt/√2,    90°: Pp = Pt
  double Pp = Pt;
  if (angle < 35.0)       Pp = Pt * SQRT3_OVER2;
  else if (angle < 60.0)  Pp = Pt / SQRT2;
  const double Bc = std::clamp(g.baffleCutFrac, 0.05, 0.45);
  r.Nc  = std::max(1.0, Ds * (1.0 - 2.0 * Bc) / std::max(Pp, 1e-9));
  r.Ncw = std::max(1.0, 0.8 * (Bc * Ds) / std::max(Pp, 1e-9));

  // --- Ideal crossflow h (Colburn j) ---------------------------------------
  const double Pr = shell_fluid.cp * mu / std::max(1e-9, shell_fluid.k);
  const double j  = colburn_j(Re, angle);
  //   h_id = j · cp · Gs · Pr^(−2/3) · (μ/μ_w)^0.14    [assume μw ≈ μ]
  //   Pr^(-2/3) == 1 / cbrt(Pr²); cbrt is a single native call vs pow().
  const double PrClamp = std::max(Pr, 1e-9);
  const double h_id = j * shell_fluid.cp * Gs / std::cbrt(PrClamp * PrClamp);
  r.h_ideal = h_id;

  // --- Leakage areas -------------------------------------------------------
  //   Stb (tube-baffle gap)   ≈ π · Do_eff · δtb · (Nt · (1 − Fw)) / 2
  //   Ssb (shell-baffle gap)  ≈ π · Ds · δsb · (360° − θds)/360° / 2
  //   Fw = window area fraction; approximate Fw from baffle cut angle θds.
  const double theta_ds = 2.0 * std::acos(std::clamp(1.0 - 2.0 * Bc, -1.0, 1.0)); // [rad]
  const double Fw = (theta_ds - std::sin(theta_ds)) / (2.0 * PI);                 // window area / shell area
  const double Fc = std::clamp(1.0 - 2.0 * Fw, 0.0, 1.0);                          // crossflow-region tube fraction
  const int    Nt = std::max(1, g.nTubes);
  const double Stb = PI * Do_eff * cfg.tube_baffle_gap * Nt * (1.0 + Fc) / 2.0;    // [m²]
  const double Ssb = Ds * cfg.shell_baffle_gap * (PI - 0.5 * theta_ds) / 2.0;      // [m²]

  // --- Bypass area (bundle-to-shell gap area for crossflow) ----------------
  const double Sb  = B * bypassGap;
  const double Fbp = Sb / r.Sm;

  // --- Correction factors (heat transfer) ----------------------------------
  r.Jc = std::clamp(0.55 + 0.72 * Fc, 0.52, 1.15);                                 // baffle-cut

  const double r_s  = (Ssb + Stb > 0.0) ? Ssb / (Ssb + Stb) : 0.0;                 // shell-to-total leakage
  const double r_lm = (Ssb + Stb) / r.Sm;
  r.Jl = 0.44 * (1.0 - r_s) + (1.0 - 0.44 * (1.0 - r_s)) * std::exp(-2.2 * r_lm);
  r.Jl = std::clamp(r.Jl, 0.2, 1.0);

  const double Cbh = (Re < 100.0) ? 1.35 : 1.25;                                    // laminar vs turbulent
  const double Nss = static_cast<double>(std::max(0, cfg.n_sealing_strips));
  const double ss_ratio = std::cbrt(std::max(0.0, 2.0 * Nss / std::max(r.Nc, 1.0)));
  r.Jb = std::exp(-Cbh * Fbp * (1.0 - ss_ratio));
  r.Jb = std::clamp(r.Jb, 0.5, 1.0);

  r.Js = 1.0;                                                                        // equal spacing assumed
  r.Jr = (Re < 20.0) ? std::pow(10.0 / std::max(r.Nc + r.Ncw, 1.0), 0.18) :
        (Re < 100.0) ? std::pow(10.0 / std::max(r.Nc + r.Ncw, 1.0), 0.18) *
                        (100.0 - Re) / 80.0 + (Re - 20.0) / 80.0
                     : 1.0;
  r.Jr = std::clamp(r.Jr, 0.4, 1.0);

  r.h_shell = h_id * r.Jc * r.Jl * r.Jb * r.Js * r.Jr;
  r.h_shell = std::max(1.0, r.h_shell);

  // --- Pressure-drop correction factors ------------------------------------
  //   R_l = exp[ −1.33 · (1 + r_s) · r_lm^p ], with p = 0.8 for Re≥100, 1.33 Re<100
  const double p = (Re < 100.0) ? 1.33 : 0.8;
  r.Rl = std::exp(-1.33 * (1.0 + r_s) * std::pow(std::max(r_lm, 1e-9), p));
  r.Rl = std::clamp(r.Rl, 0.15, 1.0);

  //   R_b = exp[ −Cbp · Fbp · (1 − (2·Nss/Nc)^(1/3)) ]
  //   Cbp = 4.5 (turb), 3.7 (laminar)
  const double Cbp = (Re < 100.0) ? 4.5 : 3.7;
  r.Rb = std::exp(-Cbp * Fbp * (1.0 - ss_ratio));
  r.Rb = std::clamp(r.Rb, 0.3, 1.0);

  r.Rs = 1.0;                                                                        // equal-spacing → no entrance/exit penalty

  // --- Ideal crossflow ΔP per baffle space ---------------------------------
  const double f = friction_f_bank(Re, angle);
  //   ΔP_ideal = 2 · f · Nc · Gs² / ρ_s   (Gs² term uses ρ once)
  r.dP_ideal = 2.0 * f * r.Nc * (Gs * Gs) / std::max(1e-9, shell_fluid.rho);

  // --- Window ΔP (Bell simplified): ---------------------------------------
  //   ΔP_w ≈ (2 + 0.6·Ncw) · Gw² / (2·ρ)   with Gw the geometric mean velocity
  //   Swindow ≈ Ds · Bc  (approx)
  const double Sw = std::max(1e-9, Ds * Bc * B * 0.5);
  const double Gw = m_dot_shell / Sw;
  const double dP_window = (2.0 + 0.6 * r.Ncw) * (Gw * Gw) / (2.0 * std::max(1e-9, shell_fluid.rho));

  //   Total: ΔP_s = (Nb−1)·ΔP_bc·R_b·R_l + Nb·ΔP_w·R_l + 2·ΔP_bc·(1+Ncw/Nc)·R_b·R_s
  const double dP_bc = r.dP_ideal;
  const double dP_total =
      (Nb - 1) * dP_bc * r.Rb * r.Rl
      + Nb      * dP_window * r.Rl
      + 2.0 * dP_bc * (1.0 + r.Ncw / std::max(r.Nc, 1.0)) * r.Rb * r.Rs;
  r.dP_shell = std::max(0.0, dP_total);

  return r;
}

} // namespace hx
