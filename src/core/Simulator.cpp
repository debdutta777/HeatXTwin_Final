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
  // Pass k_deposit to steady state calculation
  state_ = thermo_.steady(op_, 0.0, 0.0, foul_.params().k_deposit);
  // Initialize with small deterministic perturbations for consistency
  state_.Tc_out += 0.1; // +0.1°C initial variation
  state_.Th_out -= 0.1; // -0.1°C initial variation
}

const State &Simulator::step(double t) {
  const double dt = cfg_.dt;

  double Rf_shell = 0.0;
  double Rf_tube = 0.0;
  const double k_deposit = foul_.params().k_deposit;
  const double split_ratio = foul_.params().split_ratio;

  if (foulingEnabled_) {
    // Direct calculation of fouling resistance at time t (no lag)
    // Use the fouling model to compute Rf(t) directly
    state_.Rf = foul_.Rf(t);
    state_.Rf = std::max(0.0, std::min(state_.Rf, 0.01)); // Max 0.01 m²K/W fouling
    
    // Distribute fouling based on split_ratio
    Rf_shell = state_.Rf * split_ratio;
    Rf_tube = state_.Rf * (1.0 - split_ratio);
  } else {
    state_.Rf = 0.0;
  }
  
  // Enhanced dynamic operating conditions with stronger disturbances
  OperatingPoint dynamic_op = op_;
  
  // Apply disturbances only if NOT in steady-state mode
  if (!steadyStateMode_) {
    switch (cfg_.disturbanceType) {
      case SimConfig::DisturbanceType::SineWave:
        // Configurable sine wave disturbances
        dynamic_op.Tin_hot += cfg_.dist_sine_amp_Tin * std::sin(2.0 * M_PI * t / cfg_.dist_sine_freq_Tin) + 
                              (cfg_.dist_sine_amp_Tin * 0.5) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin / 3.0));
        
        dynamic_op.Tin_cold += (cfg_.dist_sine_amp_Tin * 0.66) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin * 1.33)) + 
                               (cfg_.dist_sine_amp_Tin * 0.33) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin * 0.5));
        
        // Flow rate variations
        dynamic_op.m_dot_hot += op_.m_dot_hot * cfg_.dist_sine_amp_flow * std::sin(2.0 * M_PI * t / cfg_.dist_sine_freq_flow) + 
                                op_.m_dot_hot * (cfg_.dist_sine_amp_flow * 0.5) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.4));
        
        dynamic_op.m_dot_cold += op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.8) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.73)) + 
                                  op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.4) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.26));
        break;

      case SimConfig::DisturbanceType::StepChange:
        // Step change
        if (t > cfg_.dist_step_time) {
            dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_step_mag_flow);
            dynamic_op.Tin_hot += cfg_.dist_step_mag_Tin;
        }
        break;

      case SimConfig::DisturbanceType::Ramp:
        // Ramp up
        if (t > cfg_.dist_ramp_start) {
            double ramp = std::min(1.0, (t - cfg_.dist_ramp_start) / std::max(1.0, cfg_.dist_ramp_duration));
            dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_ramp_mag_flow * ramp);
        }
        break;

      case SimConfig::DisturbanceType::None:
      default:
        // No disturbances
        break;
    }
  }
  // else: steadyStateMode_ = true, no disturbances added, use op_ directly
  
  // Compute target steady state for current conditions
  const State target = thermo_.steady(dynamic_op, Rf_shell, Rf_tube, k_deposit);

  // Use proper dynamic energy balance ODEs instead of first-order lags
  // Get current U and heat transfer area
  const double Ut = thermo_.U(dynamic_op.m_dot_hot, dynamic_op.m_dot_cold, Rf_shell, Rf_tube, k_deposit);
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
  // Tube side is HOT fluid, Shell side is COLD fluid (per report)
  // Pass configurable loss coefficients and deposit conductivity
  state_.dP_tube = hydro_.dP_tube(dynamic_op.m_dot_hot, Rf_tube, k_deposit, thermo_.geometry().K_minor_tube);
  state_.dP_shell = hydro_.dP_shell(dynamic_op.m_dot_cold, Rf_shell, k_deposit, thermo_.geometry().K_turns_shell);
  
  return state_;
}

} // namespace hx
