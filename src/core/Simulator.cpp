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

void Simulator::reset(const OperatingPoint &op0) { 
  op_ = op0; 
  state_ = thermo_.steady(op_, 0.0, 0.0);
  // Initialize with small deterministic perturbations for consistency
  state_.Tc_out += 0.1; // +0.1°C initial variation
  state_.Th_out -= 0.1; // -0.1°C initial variation
}

const State &Simulator::step(double t) {
  // Dynamic fouling resistance with time lag
  const double Rf_target = foul_.Rf(t);
  const double dt = cfg_.dt;
  
  // Fouling builds up slowly (time constant: 10 minutes)
  state_.Rf += (Rf_target - state_.Rf) * (dt / 600.0);
  const double Rf_shell = state_.Rf * 0.5;
  const double Rf_tube = state_.Rf * 0.5;
  
  // Enhanced dynamic operating conditions with stronger disturbances
  OperatingPoint dynamic_op = op_;
  
  // More realistic temperature variations (±3°C range)
  dynamic_op.Tin_hot += 3.0 * std::sin(2.0 * M_PI * t / 900.0) + 1.5 * std::sin(2.0 * M_PI * t / 300.0);
  dynamic_op.Tin_cold += 2.0 * std::sin(2.0 * M_PI * t / 1200.0) + 1.0 * std::sin(2.0 * M_PI * t / 450.0);
  
  // Stronger flow rate variations (±15% range)
  dynamic_op.m_dot_hot += op_.m_dot_hot * 0.15 * std::sin(2.0 * M_PI * t / 1500.0) + 
                          op_.m_dot_hot * 0.08 * std::sin(2.0 * M_PI * t / 600.0);
  dynamic_op.m_dot_cold += op_.m_dot_cold * 0.12 * std::sin(2.0 * M_PI * t / 1100.0) + 
                            op_.m_dot_cold * 0.06 * std::sin(2.0 * M_PI * t / 400.0);
  
  // Compute target steady state for current conditions
  const State target = thermo_.steady(dynamic_op, Rf_shell, Rf_tube);

  // Realistic thermal inertia with longer time constants
  const double tau_temp = 120.0;  // 2-minute temperature time constant
  const double tau_U = 180.0;     // 3-minute U coefficient time constant  
  const double tau_Q = 90.0;      // 1.5-minute heat duty time constant
  
  // Temperature response with thermal mass effects - with bounds checking
  state_.Tc_out += (target.Tc_out - state_.Tc_out) * (dt / tau_temp);
  state_.Th_out += (target.Th_out - state_.Th_out) * (dt / tau_temp);
  
  // Ensure reasonable temperature bounds (0-200°C)
  state_.Tc_out = std::max(0.0, std::min(200.0, state_.Tc_out));
  state_.Th_out = std::max(0.0, std::min(200.0, state_.Th_out));
  
  // Heat transfer coefficient with fouling and flow dynamics - with bounds checking
  state_.U += (target.U - state_.U) * (dt / tau_U);
  state_.U = std::max(10.0, std::min(10000.0, state_.U)); // Reasonable U range
  
  // Heat duty with thermal lag and capacity effects - with bounds checking
  state_.Q += (target.Q - state_.Q) * (dt / tau_Q);
  state_.Q = std::max(0.0, std::min(1e6, state_.Q)); // Reasonable Q range
  
  // Fouling resistance bounds
  state_.Rf = std::max(0.0, std::min(0.01, state_.Rf)); // Max 0.01 m²K/W fouling
  
  // Pressure drops respond quickly to flow changes
  state_.dP_tube = hydro_.dP_tube(dynamic_op.m_dot_cold, Rf_tube);
  state_.dP_shell = hydro_.dP_shell(dynamic_op.m_dot_hot, Rf_shell);
  
  return state_;
}

} // namespace hx
