#include "Hydraulics.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

static constexpr double PI = 3.14159265358979323846;

Hydraulics::Hydraulics(const Geometry &g, const Fluid &hot, const Fluid &cold) : g_(g), hot_(hot), cold_(cold) {}

static double tube_area(double Di) { return PI * (Di * Di) / 4.0; }

static double friction_factor(double Re) {
  if (Re < 1e-12)
    return 1.0; // degenerate
  if (Re < 2300.0)
    return 64.0 / std::max(Re, 1.0);
  // Blasius for smooth turbulent
  return 0.316 * std::pow(Re, -0.25);
}

double Hydraulics::dP_tube(double m_dot_cold, double Rf_tube) const {
  // Optionally reduce diameter with fouling thickness mapping (simple): thickness ~ Rf * k_eff
  const double k_dep = 0.5; // rough mapping [m/(m^2K/W)]
  const double t_dep = std::max(0.0, Rf_tube) * k_dep;
  const double Di_eff = std::max(1e-6, g_.Di - 2.0 * t_dep);

  const double A_total = tube_area(Di_eff) * std::max(1, g_.nTubes);
  const double v = (m_dot_cold / cold_.rho) / std::max(A_total, 1e-12);
  const double Re = cold_.rho * v * Di_eff / std::max(cold_.mu, 1e-12);
  const double f = friction_factor(Re);
  const double L = g_.L;
  const double dp_fric = f * (L / Di_eff) * 0.5 * cold_.rho * v * v;
  const double K_minor = 1.5; // two bends, entrance/exit lumped
  const double dp_minor = K_minor * 0.5 * cold_.rho * v * v;
  return std::max(0.0, dp_fric + dp_minor);
}

double Hydraulics::dP_shell(double m_dot_hot, double Rf_shell) const {
  // Approximate shell-side as channel flow with equivalent diameter
  const double De = std::max(1e-6, g_.shellID - g_.Do);
  const double As = (PI * g_.shellID * g_.baffleSpacing);
  const double v = (m_dot_hot / hot_.rho) / std::max(As, 1e-12);
  const double Re = hot_.rho * v * De / std::max(hot_.mu, 1e-12);
  const double f = friction_factor(Re) * (1.0 + 5.0 * std::max(0.0, Rf_shell) / 1e-4); // fouling ups friction
  const double Leq = g_.nBaffles * std::max(g_.baffleSpacing, 1e-6);
  const double dp_fric = f * (Leq / De) * 0.5 * hot_.rho * v * v;
  const double K_turns = 2.0 * g_.nBaffles * 0.2; // window/turn losses
  const double dp_minor = K_turns * 0.5 * hot_.rho * v * v;
  return std::max(0.0, dp_fric + dp_minor);
}

} // namespace hx
