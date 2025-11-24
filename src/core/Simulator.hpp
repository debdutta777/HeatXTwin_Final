#pragma once

#include "Model.hpp"

namespace hx {

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
};

/** \brief Simulator advances the model in time with simple first-order lags to emulate dynamics. */
class Simulator {
public:
  Simulator(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul, const SimConfig &cfg);

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
  const Thermo &thermo_;
  const Hydraulics &hydro_;
  const Fouling &foul_;
  SimConfig cfg_;
  OperatingPoint op_{};
  State state_{};
  bool steadyStateMode_{false};  // true = no disturbances, false = dynamic with disturbances
  bool foulingEnabled_{true};
};

} // namespace hx
