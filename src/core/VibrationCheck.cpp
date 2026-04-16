#include "VibrationCheck.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hx {

namespace {

constexpr double PI = 3.14159265358979323846;

/** End-condition coefficient for the 1st bending mode of an Euler-Bernoulli beam.
 *    fn = (C / (2π L²)) · √(E·I / m_e)
 *    with L equal to the unsupported span.
 */
double endConditionCoeff(int end_support) {
  switch (end_support) {
    case 0:  return 9.870;   // pinned-pinned   (π²)
    case 2:  return 15.418;  // clamped-pinned
    case 1:
    default: return 22.373;  // clamped-clamped
  }
}

const char *endConditionLabel(int end_support) {
  switch (end_support) {
    case 0:  return "pinned-pinned";
    case 2:  return "clamped-pinned";
    case 1:
    default: return "clamped-clamped";
  }
}

VibrationResult::Status vortexStatus(double fr) {
  if (fr >= 0.8) return VibrationResult::Status::Fail;
  if (fr >= 0.5) return VibrationResult::Status::Marginal;
  return VibrationResult::Status::Safe;
}

VibrationResult::Status fluidElasticStatus(double vr) {
  if (vr >= 1.0) return VibrationResult::Status::Fail;
  if (vr >= 0.5) return VibrationResult::Status::Marginal;
  return VibrationResult::Status::Safe;
}

VibrationResult::Status acousticStatus(double margin) {
  if (margin < 0.05) return VibrationResult::Status::Fail;
  if (margin < 0.20) return VibrationResult::Status::Marginal;
  return VibrationResult::Status::Safe;
}

} // anonymous namespace

VibrationResult computeVibration(const Geometry &g,
                                  const Fluid    &shell_fluid,
                                  const Fluid    &tube_fluid,
                                  double          m_dot_shell,
                                  const VibrationConfig &cfg) {
  VibrationResult r{};

  const double Do  = std::max(1e-6, g.Do);
  const double Di  = std::max(1e-6, g.Di);
  const double Ds  = std::max(1e-6, g.shellID);
  const double Pt  = std::max(1e-6, g.pitch);
  const double B   = std::max(1e-6, g.baffleSpacing);

  // --- Effective unsupported span (longest between supports) --------------
  r.L_eff = B;

  // --- Moment of inertia & flexural rigidity ------------------------------
  r.I_area = (PI / 64.0) * (std::pow(Do, 4) - std::pow(Di, 4));
  r.EI     = cfg.E_tube * r.I_area;

  // --- Effective mass per unit length (tube metal + contained + added) ----
  const double A_metal   = (PI / 4.0) * (Do * Do - Di * Di);
  const double A_inside  = (PI / 4.0) * (Di * Di);
  const double A_outside = (PI / 4.0) * (Do * Do);
  const double m_metal    = cfg.rho_tube      * A_metal;
  const double m_internal = tube_fluid.rho    * A_inside;
  const double m_added    = cfg.Cm_added_mass * shell_fluid.rho * A_outside;
  r.m_eff = m_metal + m_internal + m_added;

  // --- Natural frequency (1st mode) ---------------------------------------
  r.C_end = endConditionCoeff(cfg.end_support);
  r.fn    = (r.C_end / (2.0 * PI * r.L_eff * r.L_eff))
          * std::sqrt(r.EI / std::max(r.m_eff, 1e-9));

  // --- Shell-side crossflow velocity (Kern-style Sm so check is independent
  //     of chosen shell-side correlation) -------------------------------
  r.Sm = (Ds / Pt) * (Pt - Do) * B;
  r.Sm = std::max(r.Sm, 1e-9);
  r.V_cross = (m_dot_shell / std::max(shell_fluid.rho, 1e-9)) / r.Sm;

  // --- Vortex-shedding frequency -----------------------------------------
  r.fv = cfg.strouhal * r.V_cross / Do;
  r.freq_ratio = (r.fn > 1e-6) ? r.fv / r.fn : 0.0;

  // --- Fluid-elastic instability (Connors) -------------------------------
  //     V_crit = K · fn · Do · √( m_e · δ / (ρ · Do²) )
  const double mass_damp = (r.m_eff * cfg.damping_log_decrement)
                         / (std::max(shell_fluid.rho, 1e-9) * Do * Do);
  r.V_crit  = cfg.connors_K * r.fn * Do * std::sqrt(std::max(mass_damp, 0.0));
  r.V_ratio = (r.V_crit > 1e-9) ? r.V_cross / r.V_crit : 1e9;

  // --- Acoustic: 1st transverse mode of shell ---------------------------
  //     Estimate speed of sound: auto-pick a liquid (~1500) or gas (~340)
  //     band based on shell-fluid density unless overridden.
  if (cfg.sound_speed_override > 0.0) {
    r.c_sound = cfg.sound_speed_override;
  } else if (shell_fluid.rho > 100.0) {
    r.c_sound = 1500.0;  // water-like liquid
  } else {
    r.c_sound = 340.0;   // air-like gas
  }
  //     First transverse acoustic mode: f_a = c / (2·Ds)
  r.f_acoustic = r.c_sound / (2.0 * Ds);
  const double fMax = std::max(r.f_acoustic, std::max(r.fv, 1e-9));
  r.acoustic_margin = std::fabs(r.f_acoustic - r.fv) / fMax;

  // --- Status evaluation ------------------------------------------------
  r.vortex       = vortexStatus(r.freq_ratio);
  r.fluidElastic = fluidElasticStatus(r.V_ratio);
  r.acoustic     = acousticStatus(r.acoustic_margin);

  // Overall = worst of the three
  int worst = static_cast<int>(r.vortex);
  worst = std::max(worst, static_cast<int>(r.fluidElastic));
  worst = std::max(worst, static_cast<int>(r.acoustic));
  r.overall = static_cast<VibrationResult::Status>(worst);

  // --- Human-readable summary ------------------------------------------
  const char *verdict = (r.overall == VibrationResult::Status::Fail)     ? "FAIL"
                      : (r.overall == VibrationResult::Status::Marginal) ? "MARGINAL"
                                                                          : "SAFE";
  char buf[512];
  std::snprintf(buf, sizeof(buf),
                "TEMA FIV verdict: %s. fn=%.1f Hz (%s span = %.3f m), "
                "fv=%.2f Hz (fv/fn = %.2f), V_cross=%.3f m/s, V_crit=%.3f m/s "
                "(V/V_crit = %.2f), f_acoustic=%.1f Hz.",
                verdict, r.fn, endConditionLabel(cfg.end_support), r.L_eff,
                r.fv, r.freq_ratio, r.V_cross, r.V_crit, r.V_ratio,
                r.f_acoustic);
  r.summary = buf;
  return r;
}

} // namespace hx
