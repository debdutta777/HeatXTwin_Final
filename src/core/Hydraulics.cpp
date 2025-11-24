#include "Hydraulics.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

static constexpr double PI = 3.14159265358979323846;

Hydraulics::Hydraulics(const Geometry &g, const Fluid &hot, const Fluid &cold) : g_(g), hot_(hot), cold_(cold) {}

static double tube_area(double Di) { return PI * (Di * Di) / 4.0; }

static double friction_factor_tube(double Re) {
  if (Re < 1e-12)
    return 1.0; // degenerate
  if (Re < 2300.0)
    return 64.0 / std::max(Re, 1.0);
  // Blasius correlation for smooth turbulent flow: f = 0.3164 * Re^-0.25
  // Valid for 4000 < Re < 10^5
  return 0.3164 * std::pow(Re, -0.25);
}

static double friction_factor_shell(double Re) {
  if (Re < 1e-12) return 1.0;
  // Shell-side flow is cross-flow over tube banks.
  // Blasius (pipe flow) is incorrect here.
  // Using a standard approximation for industrial shell-side flow (e.g. Kern/Grimison type)
  // Typical range: f ~ 0.2 to 0.5 for turbulent flow
  // Approximation: f = 1.44 * Re^-0.15 (Kern-like behavior)
  return 1.44 * std::pow(Re, -0.15);
}

double Hydraulics::dP_tube(double m_dot_hot, double Rf_tube, double k_deposit, double K_minor) const {
  // Optionally reduce diameter with fouling thickness mapping (simple): thickness ~ Rf * k_eff
  const double t_dep = std::max(0.0, Rf_tube) * k_deposit;
  const double Di_eff = std::max(1e-6, g_.Di - 2.0 * t_dep);

  const double A_total = tube_area(Di_eff) * std::max(1, g_.nTubes);
  const double v = (m_dot_hot / hot_.rho) / std::max(A_total, 1e-12);
  const double Re = hot_.rho * v * Di_eff / std::max(hot_.mu, 1e-12);
  const double f = friction_factor_tube(Re);
  const double L = g_.L;
  const double dp_fric = f * (L / Di_eff) * 0.5 * hot_.rho * v * v;
  
  // Minor loss coefficient (entrance/exit/bends)
  // Passed as argument (configurable)
  const double dp_minor = K_minor * 0.5 * hot_.rho * v * v;
  return std::max(0.0, dp_fric + dp_minor);
}

double Hydraulics::dP_shell(double m_dot_cold, double Rf_shell, double k_deposit, double K_turns) const {
  // Report: "fouling changes the shell-side hydraulic diameter... recalculate Re and velocity"
  
  // 1. Calculate effective diameter with fouling
  const double delta_shell = std::max(0.0, Rf_shell) * k_deposit;
  
  const double De_clean = std::max(1e-6, g_.shellID - g_.Do);
  // Reduce hydraulic diameter by 2*delta (fouling on both sides of the gap)
  const double De_eff = std::max(1e-6, De_clean - 2.0 * delta_shell);
  
  // 2. Recalculate Flow Area and Velocity
  // Assume flow area scales with hydraulic diameter (gap)
  // Correct Shell-Side Area Calculation (Kern Method)
  const double As_clean = (g_.shellID / std::max(g_.pitch, 1e-6)) * (g_.pitch - g_.Do) * g_.baffleSpacing;
  const double As_eff = As_clean * (De_eff / De_clean);
  
  const double v_eff = (m_dot_cold / cold_.rho) / std::max(As_eff, 1e-12);
  
  // 3. Recalculate Reynolds Number
  const double Re_eff = cold_.rho * v_eff * De_eff / std::max(cold_.mu, 1e-12);
  
  // 4. Calculate Friction Factor (Use Shell-Side Correlation)
  const double f = friction_factor_shell(Re_eff); 
  
  // 5. Calculate Pressure Drop
  const double Leq = g_.nBaffles * std::max(g_.baffleSpacing, 1e-6);
  const double dp_fric = f * (Leq / De_eff) * 0.5 * cold_.rho * v_eff * v_eff;
  
  // Turn/baffle loss coefficient
  // Passed as argument (configurable)
  const double dp_minor = K_turns * 0.5 * cold_.rho * v_eff * v_eff;
  
  return std::max(0.0, dp_fric + dp_minor);
}

} // namespace hx
