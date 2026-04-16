#pragma once

#include "Model.hpp"
#include "ControllerPID.hpp"
#include "FluidLibrary.hpp"
#include "Scenario.hpp"
#include <limits>
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

  // --- Feed-forward (B5) ----------------------------------------------------
  //  u_total  =  u_bias + u_PID(sp - Tc_out)  +  u_FF(measured disturbances)
  //  u_FF     =  k_ff_Tin  · (Tin_hot  - Tin_hot_nom )
  //           +  k_ff_flow · (m_hot    - m_hot_nom   )
  //  When ff_auto_energy_balance is true, both gains are rewritten at reset()
  //  from the nominal energy balance on the operating point:
  //      m_c_nom = m_h_nom * cp_h * (Tin_hot_nom - Th_out_target)
  //                                  / (cp_c * (SP - Tin_cold_nom))
  //  and ∂m_c/∂Tin_hot, ∂m_c/∂m_h are taken analytically.
  bool   ff_enabled               = false;
  double ff_k_Tin                 = 0.0;   // [kg/s per K]
  double ff_k_flow                = 0.0;   // [kg/s per kg/s]   (dimensionless-like)
  double ff_Tin_hot_nom           = std::numeric_limits<double>::quiet_NaN();   // auto-capture on reset() if NaN
  double ff_m_dot_hot_nom         = std::numeric_limits<double>::quiet_NaN();   // auto-capture on reset() if NaN
  bool   ff_auto_energy_balance   = false; // if true, k_ff_* are derived at reset()

  // --- Cascade / actuator lag (B5) -----------------------------------------
  //  Simulates the secondary "inner" loop of a cascade architecture: a valve
  //  with first-order dynamics tracks the master-loop commanded cold flow.
  //     dm/dt = (m_cmd - m) / τ_valve
  //  When cascade is disabled or τ = 0, the exchanger sees the commanded flow
  //  instantly (legacy behaviour).
  bool   cascade_enabled = false;
  double tau_valve       = 0.0;            // [s] actuator time constant
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

  // Shell-side correlation: Kern (compact, fast) or Bell–Delaware (segmented,
  // industry-standard; applies Jc / Jl / Jb / Js / Jr corrections to an
  // ideal-crossflow base).  Set via the "Shell-side method" combo-box.
  ShellSideMethod shellMethod = ShellSideMethod::Kern;

  // Finite-volume axial discretization.
  //   numAxialCells <= 1  →  lumped model (legacy behavior)
  //   numAxialCells  > 1  →  N-cell plug-flow finite volume with proper upwind advection
  //                          and per-cell heat transfer (plots a real T(x) profile).
  int numAxialCells = 20;

  // Scripted timeline: events whose `fired` flag is false and whose trigger
  // time has passed are applied (in declaration order) at the top of every
  // Simulator::step() call.  Leave empty for no scripting.
  Scenario scenario;
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

  // Cascade/actuator inner state (valve position): actual cold flow reaching
  // the exchanger after the first-order valve lag.  NaN  ⇒  actuator inactive.
  double actuator_m_dot_cold_{std::numeric_limits<double>::quiet_NaN()};

  // Cached FF gains resolved at reset() (honours ff_auto_energy_balance).
  double ff_k_Tin_eff_{0.0};
  double ff_k_flow_eff_{0.0};
  double ff_Tin_hot_nom_eff_{0.0};
  double ff_m_dot_hot_nom_eff_{0.0};

  void resolveFeedForwardGains();
  double computeFeedForward(double Tin_hot_meas, double m_dot_hot_meas) const;

  // Axial (finite-volume) cell state — populated when cfg_.numAxialCells > 1.
  // Th_cell_[0]   is adjacent to the hot-inlet header (x = 0).
  // Th_cell_[N-1] is adjacent to the hot-outlet header (x = L).
  // Cold indexing follows the same spatial convention; the cold-flow direction
  // (counter vs. parallel) only changes which end feeds the inlet boundary.
  std::vector<double> Th_cell_;
  std::vector<double> Tc_cell_;

  void initAxialProfile();
  void stepAxial(double t, double dt);
  void stepLumped(double t, double dt);
  void applyScenarioEvents(double t);
};

} // namespace hx
