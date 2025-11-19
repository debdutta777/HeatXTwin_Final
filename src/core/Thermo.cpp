#include "Thermo.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

static constexpr double PI = 3.14159265358979323846;

Thermo::Thermo(const Geometry &g, const Fluid &hot, const Fluid &cold) : g_(g), hot_(hot), cold_(cold) {}

static double tube_cross_section(double Di) { return PI * (Di * Di) / 4.0; }

static double reynolds(double m_dot, double rho, double mu, double D, int nTubes) {
  const double A = tube_cross_section(D) * std::max(1, nTubes);
  const double v = (m_dot / rho) / A; // bulk velocity per tube if single tube; approximate per bundle
  return rho * v * D / mu;
}

static double prandtl(double cp, double mu, double k) { return cp * mu / k; }

double Thermo::h_tube(double m_dot_cold) const {
  // Use Dittus–Boelter for turbulent, laminar fallback Nu=4.36
  const double Re = reynolds(m_dot_cold, cold_.rho, cold_.mu, g_.Di, g_.nTubes);
  const double Pr = prandtl(cold_.cp, cold_.mu, cold_.k);
  double Nu;
  if (Re < 2300.0) {
    Nu = 4.36; // constant wall temperature
  } else {
    // Dittus-Boelter: n=0.4 for heating (fluid being heated), n=0.3 for cooling
    // Tube side (cold fluid) is being heated, so use n=0.4
    Nu = 0.023 * std::pow(Re, 0.8) * std::pow(Pr, 0.4);
  }
  return Nu * cold_.k / g_.Di;
}

double Thermo::h_shell(double m_dot_hot) const {
  // Zhukauskas tube-bank correlation for shell-side cross-flow
  const double De = g_.shellID - g_.Do; // crude gap estimate
  const double As = (PI * g_.shellID * g_.baffleSpacing); // cross-flow area per baffle pitch (very rough)
  const double v = (m_dot_hot / hot_.rho) / std::max(As, 1e-6);
  const double Re = hot_.rho * v * std::max(De, 1e-6) / hot_.mu;
  const double Pr = prandtl(hot_.cp, hot_.mu, hot_.k);
  
  // Zhukauskas correlation: Nu = C * Re^m * Pr^n * (Pr/Pr_w)^0.25
  // For typical range, using C=0.27, m=0.63, n=0.36
  const double C = 0.27;
  const double m = 0.63;
  const double n = 0.36; // Updated from 0.33 to match Zhukauskas correlation
  
  // Assume Pr_w ≈ Pr for simplicity (viscosity correction term)
  // In full implementation, would evaluate properties at wall temperature
  const double Pr_ratio = 1.0; // (Pr/Pr_w)^0.25 ≈ 1.0 for similar temperatures
  
  const double Nu = C * std::pow(std::max(Re, 1.0), m) * std::pow(Pr, n) * Pr_ratio;
  return Nu * hot_.k / std::max(De, 1e-6);
}

double Thermo::U(double m_dot_hot, double m_dot_cold, double Rf_shell, double Rf_tube) const {
  const double ht = std::max(h_tube(m_dot_cold), 1.0);
  const double hs = std::max(h_shell(m_dot_hot), 1.0);
  const double Rw = g_.wall_thickness / std::max(g_.wall_k, 1e-9);
  
  // Standard series resistance network (no empirical correction factor)
  // 1/U = 1/h_shell + R_wall + (1/h_tube)*(Di/Do) + Rf_shell + Rf_tube
  const double invU = (1.0 / hs) + Rw + (1.0 / ht) * (g_.Di / g_.Do) + Rf_shell + Rf_tube;
  
  return 1.0 / std::max(invU, 1e-9);
}

State Thermo::steady(const OperatingPoint &op, double Rf_shell, double Rf_tube) const {
  State s{};

  // Compute U and area
  const double Uo = U(op.m_dot_hot, op.m_dot_cold, Rf_shell, Rf_tube);
  const double A = g_.areaOuter();

  // Heat capacity rates
  const double Ch = op.m_dot_hot * hot_.cp;
  const double Cc = op.m_dot_cold * cold_.cp;
  const double Cmin = std::min(Ch, Cc);
  const double Cmax = std::max(Ch, Cc);
  const double Cr = Cmin / std::max(Cmax, 1e-12);

  // NTU and effectiveness for counter-flow
  const double NTU = (Uo * A) / std::max(Cmin, 1e-12);
  const double one_minus_Cr = (1.0 - Cr);
  const double exp_term = std::exp(-NTU * one_minus_Cr);
  const double eps = (std::abs(one_minus_Cr) < 1e-12) ? (NTU / (1.0 + NTU)) : ((1.0 - exp_term) / (1.0 - Cr * exp_term));

  const double Th_in = op.Tin_hot;
  const double Tc_in = op.Tin_cold;
  const double Q = eps * Cmin * (Th_in - Tc_in);

  const double Tc_out = Tc_in + Q / std::max(Cc, 1e-12);
  const double Th_out = Th_in - Q / std::max(Ch, 1e-12);

  s.Tc_out = Tc_out;
  s.Th_out = Th_out;
  s.Q = Q;
  s.U = Uo;
  s.Rf = Rf_shell + Rf_tube;
  s.dP_tube = 0.0;  // filled by Hydraulics component in integration
  s.dP_shell = 0.0; // filled by Hydraulics component in integration
  return s;
}

} // namespace hx
