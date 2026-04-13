#pragma once

#include "Model.hpp"
#include "ControllerPID.hpp"
#include "FluidLibrary.hpp"
#include <memory>

namespace hx {

/** \brief Configuration for the optional PID loop that regulates cold-side
 *  mass flow (manipulated variable) to track a desired cold outlet temperature
 *  (process variable).
 */
struct PidConfig {
  bool enabled = false;
  double setpoint_Tc_out = 45.0;   // [C]
  double kp = 0.05;                // [kg/s/K]
  double ki = 0.005;               // [kg/s/K/s]
  double kd = 0.0;                 // [kg/s·s/K]
  double u_min = 0.1;              // [kg/s] minimum cold flow
  double u_max = 5.0;              // [kg/s] maximum cold flow
  double rate_limit = 0.5;         // [kg/s/s]
};

struct SimConfig {
  double dt;    // [s] time step
  double tEnd;  // [s] end time
  int cells;    // for multi-cell later
  bool multicell;
  Limits limits;
  
  // Dynamic simulation parameters (holdup masses for ODE integration)
  double Mh;    // [kg] hot-side fluid holdup mass
  double Mc;    // [kg] cold-side fluid holdup mass

  // Disturbance configuration
  enum class DisturbanceType {
      None,
      SineWave,
      StepChange,
      Ramp
  } disturbanceType = DisturbanceType::SineWave;

  // Configurable disturbance parameters
  double dist_sine_amp_Tin = 3.0;      // Amplitude for Tin sine wave [C]
  double dist_sine_freq_Tin = 900.0;   // Period for Tin sine wave [s]
  double dist_sine_amp_flow = 0.15;    // Amplitude for flow sine wave (fraction)
  double dist_sine_freq_flow = 1500.0; // Period for flow sine wave [s]
  
  double dist_step_time = 300.0;       // Time for step change [s]
  double dist_step_mag_flow = 0.1;     // Magnitude for flow step change (fraction)
  double dist_step_mag_Tin = 5.0;      // Magnitude for Tin step change [C]
  
  double dist_ramp_start = 100.0;      // Ramp start time [s]
  double dist_ramp_duration = 600.0;   // Ramp duration [s]
  double dist_ramp_mag_flow = 0.2;     // Ramp magnitude (fraction)

  PidConfig pid;

  // Temperature-dependent property updates. When either preset is not Custom,
  // the simulator re-evaluates ρ/μ/cₚ/k at the mean fluid temperature on every
  // step and updates the Thermo instance accordingly.
  FluidPreset hotPreset  = FluidPreset::Custom;
  FluidPreset coldPreset = FluidPreset::Custom;
  Fluid hotCustom{};   // fallback when hotPreset == Custom
  Fluid coldCustom{};  // fallback when coldPreset == Custom

  FlowArrangement arrangement = FlowArrangement::CounterFlow;
};

/** \brief Simulator advances the model in time with simple first-order lags to emulate dynamics. */
class Simulator {
public:
  Simulator(Thermo &thermo, const Hydraulics &hydro, const Fouling &foul, const SimConfig &cfg);

  void reset(const OperatingPoint &op0);
  [[nodiscard]] const State &step(double t);
  void updateOperatingPoint(const OperatingPoint &newOp) { op_ = newOp; }
  void setSteadyStateMode(bool enabled);
  void setFoulingEnabled(bool enabled);

  template <class F>
  void run(const OperatingPoint & /*schedule*/, F onSample) {
    double t = 0.0;
    while (t <= cfg_.tEnd) {
      [[maybe_unused]] const auto& currentState = step(t);
      if (!onSample(t, state_)) { // Allow callback to request early termination
        break;
      }
      t += cfg_.dt;
    }
  }

private:
  Thermo &thermo_;
  const Hydraulics &hydro_;
  const Fouling &foul_;
  SimConfig cfg_;
  OperatingPoint op_{};
  State state_{};
  bool steadyStateMode_{false};  // true = no disturbances, false = dynamic with disturbances
  bool foulingEnabled_{true};
  std::unique_ptr<ControllerPID> pid_;  // allocated on reset() when pid.enabled
};

} // namespace hx
