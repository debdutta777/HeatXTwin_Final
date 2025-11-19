#include "Simulator.hpp"

#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace hx {

Simulator::Simulator(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul, const SimConfig &cfg)
    : thermo_(thermo), hydro_(hydro), foul_(foul), cfg_(cfg) {}

void Simulator::setSteadyStateMode(bool enabled) {
  steadyStateMode_ = enabled;
}

void Simulator::setFoulingEnabled(bool enabled) {
  foulingEnabled_ = enabled;
  if (!foulingEnabled_) {
    state_.Rf = 0.0;
  }
}

void Simulator::reset(const OperatingPoint &op0) { 
  op_ = op0; 
  state_ = thermo_.steady(op_, 0.0, 0.0);
  // Initialize with small deterministic perturbations for consistency
  state_.Tc_out += 0.1; // +0.1°C initial variation
  state_.Th_out -= 0.1; // -0.1°C initial variation
}

const State &Simulator::step(double t) {
  const double dt = cfg_.dt;

  double Rf_shell = 0.0;
  double Rf_tube = 0.0;

  if (foulingEnabled_) {
    // Direct calculation of fouling resistance at time t (no lag)
    // Use the fouling model to compute Rf(t) directly
    state_.Rf = foul_.Rf(t);
    state_.Rf = std::max(0.0, std::min(state_.Rf, 0.01)); // Max 0.01 m²K/W fouling
    Rf_shell = state_.Rf * 0.5;
    Rf_tube = state_.Rf * 0.5;
  } else {
    state_.Rf = 0.0;
  }
  
  // Enhanced dynamic operating conditions with stronger disturbances
  OperatingPoint dynamic_op = op_;
  
  // Apply disturbances only if NOT in steady-state mode
  if (!steadyStateMode_) {
    // More realistic temperature variations (±3°C range)
    dynamic_op.Tin_hot += 3.0 * std::sin(2.0 * M_PI * t / 900.0) + 1.5 * std::sin(2.0 * M_PI * t / 300.0);
    dynamic_op.Tin_cold += 2.0 * std::sin(2.0 * M_PI * t / 1200.0) + 1.0 * std::sin(2.0 * M_PI * t / 450.0);
    
    // Stronger flow rate variations (±15% range)
    dynamic_op.m_dot_hot += op_.m_dot_hot * 0.15 * std::sin(2.0 * M_PI * t / 1500.0) + 
                            op_.m_dot_hot * 0.08 * std::sin(2.0 * M_PI * t / 600.0);
    dynamic_op.m_dot_cold += op_.m_dot_cold * 0.12 * std::sin(2.0 * M_PI * t / 1100.0) + 
                              op_.m_dot_cold * 0.06 * std::sin(2.0 * M_PI * t / 400.0);
  }
  // else: steadyStateMode_ = true, no disturbances added, use op_ directly
  
  // Compute target steady state for current conditions
  const State target = thermo_.steady(dynamic_op, Rf_shell, Rf_tube);

  // Use proper dynamic energy balance ODEs instead of first-order lags
  // Get current U and heat transfer area
  const double Ut = thermo_.U(dynamic_op.m_dot_hot, dynamic_op.m_dot_cold, Rf_shell, Rf_tube);
  const double A = thermo_.geometry().areaOuter();
  
  // Instantaneous heat transfer based on current outlet temperatures
  const double Qt = Ut * A * (state_.Th_out - state_.Tc_out);
  
  // Heat capacity rates
  const double Ch = dynamic_op.m_dot_hot * thermo_.hot().cp;
  const double Cc = dynamic_op.m_dot_cold * thermo_.cold().cp;
  
  // Thermal capacitances (holdup mass × specific heat)
  const double Cth_h = cfg_.Mh * thermo_.hot().cp;
  const double Cth_c = cfg_.Mc * thermo_.cold().cp;
  
  // Dynamic energy balance ODEs (explicit Euler integration)
  // Hot side: Cth_h * dTh_out/dt = m_h*cp_h*(Th_in - Th_out) - Q
  const double dTh_dt = (Ch * (dynamic_op.Tin_hot - state_.Th_out) - Qt) / std::max(Cth_h, 1e-12);
  
  // Cold side: Cth_c * dTc_out/dt = m_c*cp_c*(Tc_in - Tc_out) + Q
  const double dTc_dt = (Cc * (dynamic_op.Tin_cold - state_.Tc_out) + Qt) / std::max(Cth_c, 1e-12);
  
  // Update outlet temperatures
  state_.Th_out += dTh_dt * dt;
  state_.Tc_out += dTc_dt * dt;
  
  // Ensure reasonable temperature bounds (0-200°C)
  state_.Tc_out = std::max(0.0, std::min(200.0, state_.Tc_out));
  state_.Th_out = std::max(0.0, std::min(200.0, state_.Th_out));
  
  // Update state variables
  state_.U = Ut;
  state_.Q = Qt;
  state_.Q = std::max(0.0, std::min(1e6, state_.Q)); // Reasonable Q range
  
  // Pressure drops respond quickly to flow changes
  state_.dP_tube = hydro_.dP_tube(dynamic_op.m_dot_cold, Rf_tube);
  state_.dP_shell = hydro_.dP_shell(dynamic_op.m_dot_hot, Rf_shell);
  
  return state_;
}

} // namespace hx
