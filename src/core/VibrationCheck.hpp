#pragma once

#include "Types.hpp"
#include <string>

namespace hx {

/**
 * \brief TEMA-style flow-induced vibration configuration.
 *
 *  Defaults are representative of low-chrome stainless tubes in
 *  liquid-side service (e.g. TP-316).  Override for other materials.
 */
struct VibrationConfig {
  double E_tube               = 193.0e9;    // Young's modulus [Pa] (304/316 SS)
  double rho_tube             = 8000.0;     // tube material density [kg/m³]
  double Cm_added_mass        = 1.5;        // added-mass coefficient (shell fluid)
  double damping_log_decrement = 0.03;      // δ (water-in-shell typical)
  double strouhal             = 0.2;        // St for equilateral tube banks
  double connors_K            = 3.3;        // TEMA Connors constant (liquid)
  int    end_support          = 1;          // 0 = pinned-pinned, 1 = clamped-clamped, 2 = clamped-pinned
  double sound_speed_override = -1.0;       // <0 ⇒ auto-estimate from ρ (liquid vs gas)
};

/**
 * \brief Full FIV diagnostic for one baffle span.
 *
 *  All three TEMA checks are populated:
 *   1. Vortex-shedding lock-in  (fv / fn < 0.5 safe)
 *   2. Fluid-elastic instability (V_cross / V_crit < 1 safe, < 0.5 ideal)
 *   3. Acoustic resonance       (|f_a − fv|/fv > 0.2 safe)
 */
struct VibrationResult {
  // Natural frequency (1st mode, one baffle span)
  double fn       = 0.0;   // [Hz]
  double L_eff    = 0.0;   // unsupported span used [m]
  double m_eff    = 0.0;   // effective mass per unit length [kg/m]
  double I_area   = 0.0;   // bending moment of area [m⁴]
  double EI       = 0.0;   // flexural rigidity [N·m²]
  double C_end    = 0.0;   // end-condition coefficient used in fn formula

  // Crossflow excitation
  double V_cross  = 0.0;   // shell-side crossflow velocity [m/s]
  double Sm       = 0.0;   // Kern-style crossflow area [m²]
  double fv       = 0.0;   // vortex-shedding frequency [Hz]

  // Fluid-elastic instability
  double V_crit   = 0.0;   // Connors critical velocity [m/s]
  double V_ratio  = 0.0;   // V_cross / V_crit (danger ≥ 1)
  double freq_ratio = 0.0; // fv / fn        (danger ≥ 0.5)

  // Acoustic
  double c_sound      = 0.0;  // speed of sound used [m/s]
  double f_acoustic   = 0.0;  // 1st transverse acoustic mode of shell [Hz]
  double acoustic_margin = 0.0; // |f_a − fv|/max(f_a, fv)

  // Status (green = safe, yellow = marginal, red = fail)
  enum class Status { Safe = 0, Marginal = 1, Fail = 2 };
  Status vortex      = Status::Safe;
  Status fluidElastic = Status::Safe;
  Status acoustic    = Status::Safe;
  Status overall     = Status::Safe;

  std::string summary;    // one-paragraph human-readable verdict
};

/**
 * \brief Compute all TEMA vibration diagnostics for one baffle-pitch span.
 *
 *  The longest unsupported span in a segmentally baffled bundle is usually
 *  equal to the baffle spacing, so we use \c g.baffleSpacing as \c L_eff.
 *
 *  \param g             Heat-exchanger geometry.
 *  \param shell_fluid   Shell-side fluid (supplies ρ, cp for added mass & sound).
 *  \param tube_fluid    Tube-side fluid (contributes to m_e via contained mass).
 *  \param m_dot_shell   Shell-side mass flow rate [kg/s].
 *  \param cfg           Material/damping overrides (optional).
 */
VibrationResult computeVibration(const Geometry &g,
                                  const Fluid    &shell_fluid,
                                  const Fluid    &tube_fluid,
                                  double          m_dot_shell,
                                  const VibrationConfig &cfg = {});

} // namespace hx
