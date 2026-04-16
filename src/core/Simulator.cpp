#include "Simulator.hpp"

#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace hx {

Simulator::Simulator(Thermo &thermo, const Hydraulics &hydro, const Fouling &foul, const SimConfig &cfg)
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

// -----------------------------------------------------------------------------
//  Feed-forward helpers (B5).  Resolve nominal set-points and the two FF gains
//  once per reset() so the inner loop is just a multiply-and-add.
// -----------------------------------------------------------------------------
void Simulator::resolveFeedForwardGains() {
  // Auto-capture nominal disturbance values from the current operating point
  // if the user hasn't supplied explicit ones.
  ff_Tin_hot_nom_eff_   = std::isfinite(cfg_.pid.ff_Tin_hot_nom)
                           ? cfg_.pid.ff_Tin_hot_nom   : op_.Tin_hot;
  ff_m_dot_hot_nom_eff_ = std::isfinite(cfg_.pid.ff_m_dot_hot_nom)
                           ? cfg_.pid.ff_m_dot_hot_nom : op_.m_dot_hot;

  if (cfg_.pid.ff_auto_energy_balance) {
    // Analytic FF from the steady energy balance:
    //   Q = m_h cp_h (Tin_h - Th_out) = m_c cp_c (SP - Tin_c)
    //   m_c = m_h cp_h (Tin_h - Th_out_est) / (cp_c (SP - Tin_c))
    //
    // For the FF gains we differentiate the above with Th_out held at its
    // current estimate (treating it as a known, measured outlet).  This yields
    // the two partials that are exactly the required static FF gains.
    const double cp_h = thermo_.hot().cp;
    const double cp_c = thermo_.cold().cp;
    const double SP   = cfg_.pid.setpoint_Tc_out;
    const double Tin_c_nom = op_.Tin_cold;
    const double dT_cold   = std::max(SP - Tin_c_nom, 1e-3);
    const double Th_out_est = std::isfinite(state_.Th_out) ? state_.Th_out
                                                            : op_.Tin_hot - 10.0;

    // ∂m_c / ∂Tin_hot = m_h cp_h / (cp_c · (SP - Tin_cold))
    ff_k_Tin_eff_  = (ff_m_dot_hot_nom_eff_ * cp_h) / (cp_c * dT_cold);
    // ∂m_c / ∂m_h   = cp_h (Tin_h - Th_out) / (cp_c · (SP - Tin_cold))
    ff_k_flow_eff_ = (cp_h * (ff_Tin_hot_nom_eff_ - Th_out_est))
                      / (cp_c * dT_cold);
  } else {
    ff_k_Tin_eff_  = cfg_.pid.ff_k_Tin;
    ff_k_flow_eff_ = cfg_.pid.ff_k_flow;
  }
}

double Simulator::computeFeedForward(double Tin_hot_meas,
                                      double m_dot_hot_meas) const {
  if (!cfg_.pid.enabled || !cfg_.pid.ff_enabled) return 0.0;
  return ff_k_Tin_eff_  * (Tin_hot_meas  - ff_Tin_hot_nom_eff_)
       + ff_k_flow_eff_ * (m_dot_hot_meas - ff_m_dot_hot_nom_eff_);
}

void Simulator::initAxialProfile() {
  const int N = std::max(1, cfg_.numAxialCells);
  Th_cell_.assign(static_cast<size_t>(N), 0.0);
  Tc_cell_.assign(static_cast<size_t>(N), 0.0);

  // Seed with a linear interpolation between inlet and steady outlet for each
  // side.  This gives the UI a textbook-looking profile on the very first
  // frame without having to solve a tridiagonal system first.
  const double ThIn  = op_.Tin_hot;
  const double TcIn  = op_.Tin_cold;
  const double ThOut = state_.Th_out;
  const double TcOut = state_.Tc_out;

  const bool counter = (cfg_.arrangement != FlowArrangement::ParallelFlow);

  for (int i = 0; i < N; ++i) {
    const double xi = (N == 1) ? 0.5 : static_cast<double>(i) / (N - 1);
    Th_cell_[static_cast<size_t>(i)] = ThIn + (ThOut - ThIn) * xi;
    if (counter) {
      // Cold flows N-1 -> 0, so inlet (TcIn) sits at x=1 and outlet (TcOut) at x=0.
      Tc_cell_[static_cast<size_t>(i)] = TcOut + (TcIn - TcOut) * xi;
    } else {
      // Parallel: both enter together at x=0.
      Tc_cell_[static_cast<size_t>(i)] = TcIn + (TcOut - TcIn) * xi;
    }
  }

  state_.Th_axial = Th_cell_;
  state_.Tc_axial = Tc_cell_;
}

void Simulator::reset(const OperatingPoint &op0) {
  op_ = op0;
  // Pass k_deposit to steady state calculation
  state_ = thermo_.steady(op_, 0.0, 0.0, foul_.params().k_deposit, cfg_.arrangement);
  // Initialize with small deterministic perturbations for consistency
  state_.Tc_out += 0.1; // +0.1°C initial variation
  state_.Th_out -= 0.1; // -0.1°C initial variation

  // (Re)allocate PID controller with current gains.
  if (cfg_.pid.enabled) {
    pid_ = std::make_unique<ControllerPID>(
        cfg_.pid.kp, cfg_.pid.ki, cfg_.pid.kd,
        cfg_.pid.u_min, cfg_.pid.u_max, cfg_.pid.rate_limit);
    state_.pidSetpoint = cfg_.pid.setpoint_Tc_out;
    state_.pidColdFlow = op_.m_dot_cold;
    state_.pidFFterm   = cfg_.pid.ff_enabled ? 0.0
                                              : std::numeric_limits<double>::quiet_NaN();
    state_.pidFBterm   = 0.0;
    resolveFeedForwardGains();
  } else {
    pid_.reset();
    state_.pidSetpoint = std::numeric_limits<double>::quiet_NaN();
    state_.pidColdFlow = std::numeric_limits<double>::quiet_NaN();
    state_.pidFFterm   = std::numeric_limits<double>::quiet_NaN();
    state_.pidFBterm   = std::numeric_limits<double>::quiet_NaN();
  }

  // Initialise (or clear) the cascade inner-loop actuator state.
  if (cfg_.pid.enabled && cfg_.pid.cascade_enabled && cfg_.pid.tau_valve > 0.0) {
    actuator_m_dot_cold_   = op_.m_dot_cold;
    state_.pidColdFlowActual = op_.m_dot_cold;
  } else {
    actuator_m_dot_cold_     = std::numeric_limits<double>::quiet_NaN();
    state_.pidColdFlowActual = std::numeric_limits<double>::quiet_NaN();
  }

  if (cfg_.numAxialCells > 1) {
    initAxialProfile();
  } else {
    Th_cell_.clear();
    Tc_cell_.clear();
    state_.Th_axial.clear();
    state_.Tc_axial.clear();
  }
}

void Simulator::applyScenarioEvents(double t) {
  if (cfg_.scenario.empty()) return;
  for (auto &ev : cfg_.scenario) {
    if (ev.fired || t < ev.t) continue;
    switch (ev.action) {
      case ScenarioEvent::Action::SetHotFlow:
        op_.m_dot_hot = ev.value; break;
      case ScenarioEvent::Action::SetColdFlow:
        op_.m_dot_cold = ev.value; break;
      case ScenarioEvent::Action::SetHotInletT:
        op_.Tin_hot = ev.value; break;
      case ScenarioEvent::Action::SetColdInletT:
        op_.Tin_cold = ev.value; break;
      case ScenarioEvent::Action::SetPidSetpoint:
        cfg_.pid.setpoint_Tc_out = ev.value;
        state_.pidSetpoint = ev.value;
        break;
      case ScenarioEvent::Action::SetPidEnabled:
        cfg_.pid.enabled = ev.boolValue;
        if (ev.boolValue && !pid_) {
          pid_ = std::make_unique<ControllerPID>(
              cfg_.pid.kp, cfg_.pid.ki, cfg_.pid.kd,
              cfg_.pid.u_min, cfg_.pid.u_max, cfg_.pid.rate_limit);
          state_.pidSetpoint = cfg_.pid.setpoint_Tc_out;
          state_.pidColdFlow = op_.m_dot_cold;
        } else if (!ev.boolValue) {
          pid_.reset();
          state_.pidSetpoint = std::numeric_limits<double>::quiet_NaN();
          state_.pidColdFlow = std::numeric_limits<double>::quiet_NaN();
        }
        break;
      case ScenarioEvent::Action::FoulingJump:
        // Clamp so a huge negative jump just zeroes the fouling (cleaning).
        state_.Rf = std::max(0.0, std::min(0.01, state_.Rf + ev.value));
        break;
      case ScenarioEvent::Action::Mark:
        // Pure annotation — no plant effect.
        break;
    }
    ev.fired = true;
  }
}

const State &Simulator::step(double t) {
  const double dt = cfg_.dt;

  // Apply any scenario events that have matured by this point so the rest
  // of the step sees the updated operating point / controller state.
  applyScenarioEvents(t);

  // Update temperature-dependent fluid properties if either side uses a preset.
  // Mean film temperature = ((T_in + T_out) / 2).
  if (cfg_.hotPreset != FluidPreset::Custom) {
    const double Tmean = 0.5 * (op_.Tin_hot + state_.Th_out);
    thermo_.setHot(evaluateFluid(cfg_.hotPreset, Tmean, cfg_.hotCustom));
  }
  if (cfg_.coldPreset != FluidPreset::Custom) {
    const double Tmean = 0.5 * (op_.Tin_cold + state_.Tc_out);
    thermo_.setCold(evaluateFluid(cfg_.coldPreset, Tmean, cfg_.coldCustom));
  }

  if (cfg_.numAxialCells > 1) {
    stepAxial(t, dt);
  } else {
    stepLumped(t, dt);
  }
  return state_;
}

// -----------------------------------------------------------------------------
// Legacy lumped-parameter path (two ODEs, single hot/cold outlet temperature).
// Kept intact so the widget/KPI code doesn't regress and to serve as a baseline
// to compare against the finite-volume model in the report.
// -----------------------------------------------------------------------------
void Simulator::stepLumped(double t, double dt) {
  double Rf_shell = 0.0;
  double Rf_tube = 0.0;
  const double k_deposit = foul_.params().k_deposit;
  const double split_ratio = foul_.params().split_ratio;

  if (foulingEnabled_) {
    state_.Rf = foul_.Rf(t);
    state_.Rf = std::max(0.0, std::min(state_.Rf, 0.01));
    Rf_shell = state_.Rf * split_ratio;
    Rf_tube = state_.Rf * (1.0 - split_ratio);
  } else {
    state_.Rf = 0.0;
  }

  OperatingPoint dynamic_op = op_;
  if (!steadyStateMode_) {
    switch (cfg_.disturbanceType) {
      case SimConfig::DisturbanceType::SineWave:
        dynamic_op.Tin_hot += cfg_.dist_sine_amp_Tin * std::sin(2.0 * M_PI * t / cfg_.dist_sine_freq_Tin) +
                              (cfg_.dist_sine_amp_Tin * 0.5) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin / 3.0));
        dynamic_op.Tin_cold += (cfg_.dist_sine_amp_Tin * 0.66) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin * 1.33)) +
                               (cfg_.dist_sine_amp_Tin * 0.33) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_Tin * 0.5));
        dynamic_op.m_dot_hot += op_.m_dot_hot * cfg_.dist_sine_amp_flow * std::sin(2.0 * M_PI * t / cfg_.dist_sine_freq_flow) +
                                op_.m_dot_hot * (cfg_.dist_sine_amp_flow * 0.5) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.4));
        dynamic_op.m_dot_cold += op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.8) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.73)) +
                                  op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.4) * std::sin(2.0 * M_PI * t / (cfg_.dist_sine_freq_flow * 0.26));
        break;
      case SimConfig::DisturbanceType::StepChange:
        if (t > cfg_.dist_step_time) {
          dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_step_mag_flow);
          dynamic_op.Tin_hot += cfg_.dist_step_mag_Tin;
        }
        break;
      case SimConfig::DisturbanceType::Ramp:
        if (t > cfg_.dist_ramp_start) {
          const double ramp = std::min(1.0, (t - cfg_.dist_ramp_start) / std::max(1.0, cfg_.dist_ramp_duration));
          dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_ramp_mag_flow * ramp);
        }
        break;
      case SimConfig::DisturbanceType::None:
      default:
        break;
    }
  }

  if (cfg_.pid.enabled && pid_) {
    const double u_fb = pid_->update(cfg_.pid.setpoint_Tc_out, state_.Tc_out, dt);
    const double u_ff = computeFeedForward(dynamic_op.Tin_hot, dynamic_op.m_dot_hot);
    double u_cmd = u_fb + u_ff;
    u_cmd = std::clamp(u_cmd, cfg_.pid.u_min, cfg_.pid.u_max);

    // Cascade / actuator lag: m_dot actually reaching the shell lags the
    // commanded value with time constant τ_valve (explicit Euler).
    double u_actual = u_cmd;
    if (cfg_.pid.cascade_enabled && cfg_.pid.tau_valve > 0.0
         && std::isfinite(actuator_m_dot_cold_)) {
      const double tau = std::max(cfg_.pid.tau_valve, dt);
      actuator_m_dot_cold_ += dt * (u_cmd - actuator_m_dot_cold_) / tau;
      u_actual = actuator_m_dot_cold_;
      state_.pidColdFlowActual = u_actual;
    } else {
      state_.pidColdFlowActual = std::numeric_limits<double>::quiet_NaN();
    }

    dynamic_op.m_dot_cold = u_actual;
    state_.pidSetpoint  = cfg_.pid.setpoint_Tc_out;
    state_.pidColdFlow  = u_cmd;
    state_.pidFBterm    = u_fb;
    state_.pidFFterm    = cfg_.pid.ff_enabled ? u_ff
                                                : std::numeric_limits<double>::quiet_NaN();
  }

  // (Historical note: a steady-state snapshot used to be computed here
  // for diagnostics but was never consumed; removed to save one full
  // Thermo::steady() call per dynamic step.)

  const double Ut = thermo_.U(dynamic_op.m_dot_hot, dynamic_op.m_dot_cold, Rf_shell, Rf_tube, k_deposit);
  const double A = thermo_.geometry().areaOuter();

  double F = 1.0;
  if (cfg_.arrangement == FlowArrangement::ShellTube_1_2 ||
      cfg_.arrangement == FlowArrangement::ShellTube_2_4) {
    const double dT_in = dynamic_op.Tin_hot - dynamic_op.Tin_cold;
    if (dT_in > 1e-3) {
      const double dTc = std::max(state_.Tc_out - dynamic_op.Tin_cold, 1e-6);
      const double R   = (dynamic_op.Tin_hot - state_.Th_out) / dTc;
      const double P   = (state_.Tc_out - dynamic_op.Tin_cold) / dT_in;
      F = Thermo::lmtdCorrectionF(cfg_.arrangement, R, P);
    }
  } else if (cfg_.arrangement == FlowArrangement::ParallelFlow) {
    const double dT_in = std::max(dynamic_op.Tin_hot - dynamic_op.Tin_cold, 1e-6);
    F = std::clamp((state_.Th_out - state_.Tc_out) / dT_in, 0.0, 1.0);
    F = std::max(F, 0.5);
  }
  const double Qt = Ut * A * (state_.Th_out - state_.Tc_out) * F;

  const double Ch = dynamic_op.m_dot_hot * thermo_.hot().cp;
  const double Cc = dynamic_op.m_dot_cold * thermo_.cold().cp;
  const double Cth_h = cfg_.Mh * thermo_.hot().cp;
  const double Cth_c = cfg_.Mc * thermo_.cold().cp;

  const double dTh_dt = (Ch * (dynamic_op.Tin_hot - state_.Th_out) - Qt) / std::max(Cth_h, 1e-12);
  const double dTc_dt = (Cc * (dynamic_op.Tin_cold - state_.Tc_out) + Qt) / std::max(Cth_c, 1e-12);

  state_.Th_out += dTh_dt * dt;
  state_.Tc_out += dTc_dt * dt;

  state_.Tc_out = std::max(0.0, std::min(200.0, state_.Tc_out));
  state_.Th_out = std::max(0.0, std::min(200.0, state_.Th_out));

  state_.U = Ut;
  state_.Q = Qt;
  state_.Q = std::max(0.0, std::min(1e6, state_.Q));

  state_.dP_tube = hydro_.dP_tube(dynamic_op.m_dot_hot, Rf_tube, k_deposit, thermo_.geometry().K_minor_tube);
  state_.dP_shell = hydro_.dP_shell(dynamic_op.m_dot_cold, Rf_shell, k_deposit, thermo_.geometry().K_turns_shell);
}

// -----------------------------------------------------------------------------
// Finite-volume axial path: the exchanger is sliced into N streamwise cells.
// Each cell advances via upwind advection from its upstream neighbor plus a
// local heat-transfer source term (q_i = U·dA·(Th_i − Tc_i)·F).  Because the
// fluid capacitance of one cell is Mh/N (N times smaller than the lumped
// model), we CFL-substep inside the macro step to stay stable without
// tightening the outer UI time step.
// -----------------------------------------------------------------------------
void Simulator::stepAxial(double tNow, double dtMacro) {
  const int N = static_cast<int>(Th_cell_.size());
  if (N <= 1) { stepLumped(tNow, dtMacro); return; }

  double Rf_shell = 0.0;
  double Rf_tube = 0.0;
  const double k_deposit = foul_.params().k_deposit;
  const double split_ratio = foul_.params().split_ratio;

  if (foulingEnabled_) {
    state_.Rf = foul_.Rf(tNow);
    state_.Rf = std::max(0.0, std::min(state_.Rf, 0.01));
    Rf_shell = state_.Rf * split_ratio;
    Rf_tube = state_.Rf * (1.0 - split_ratio);
  } else {
    state_.Rf = 0.0;
  }

  OperatingPoint dynamic_op = op_;
  if (!steadyStateMode_) {
    switch (cfg_.disturbanceType) {
      case SimConfig::DisturbanceType::SineWave:
        dynamic_op.Tin_hot += cfg_.dist_sine_amp_Tin * std::sin(2.0 * M_PI * tNow / cfg_.dist_sine_freq_Tin) +
                              (cfg_.dist_sine_amp_Tin * 0.5) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_Tin / 3.0));
        dynamic_op.Tin_cold += (cfg_.dist_sine_amp_Tin * 0.66) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_Tin * 1.33)) +
                               (cfg_.dist_sine_amp_Tin * 0.33) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_Tin * 0.5));
        dynamic_op.m_dot_hot += op_.m_dot_hot * cfg_.dist_sine_amp_flow * std::sin(2.0 * M_PI * tNow / cfg_.dist_sine_freq_flow) +
                                op_.m_dot_hot * (cfg_.dist_sine_amp_flow * 0.5) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_flow * 0.4));
        dynamic_op.m_dot_cold += op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.8) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_flow * 0.73)) +
                                  op_.m_dot_cold * (cfg_.dist_sine_amp_flow * 0.4) * std::sin(2.0 * M_PI * tNow / (cfg_.dist_sine_freq_flow * 0.26));
        break;
      case SimConfig::DisturbanceType::StepChange:
        if (tNow > cfg_.dist_step_time) {
          dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_step_mag_flow);
          dynamic_op.Tin_hot += cfg_.dist_step_mag_Tin;
        }
        break;
      case SimConfig::DisturbanceType::Ramp:
        if (tNow > cfg_.dist_ramp_start) {
          const double ramp = std::min(1.0, (tNow - cfg_.dist_ramp_start) / std::max(1.0, cfg_.dist_ramp_duration));
          dynamic_op.m_dot_hot *= (1.0 + cfg_.dist_ramp_mag_flow * ramp);
        }
        break;
      case SimConfig::DisturbanceType::None:
      default:
        break;
    }
  }

  if (cfg_.pid.enabled && pid_) {
    const double u_fb = pid_->update(cfg_.pid.setpoint_Tc_out, state_.Tc_out, dtMacro);
    const double u_ff = computeFeedForward(dynamic_op.Tin_hot, dynamic_op.m_dot_hot);
    double u_cmd = u_fb + u_ff;
    u_cmd = std::clamp(u_cmd, cfg_.pid.u_min, cfg_.pid.u_max);

    double u_actual = u_cmd;
    if (cfg_.pid.cascade_enabled && cfg_.pid.tau_valve > 0.0
         && std::isfinite(actuator_m_dot_cold_)) {
      const double tau = std::max(cfg_.pid.tau_valve, dtMacro);
      actuator_m_dot_cold_ += dtMacro * (u_cmd - actuator_m_dot_cold_) / tau;
      u_actual = actuator_m_dot_cold_;
      state_.pidColdFlowActual = u_actual;
    } else {
      state_.pidColdFlowActual = std::numeric_limits<double>::quiet_NaN();
    }

    dynamic_op.m_dot_cold = u_actual;
    state_.pidSetpoint  = cfg_.pid.setpoint_Tc_out;
    state_.pidColdFlow  = u_cmd;
    state_.pidFBterm    = u_fb;
    state_.pidFFterm    = cfg_.pid.ff_enabled ? u_ff
                                                : std::numeric_limits<double>::quiet_NaN();
  }

  const double Ut = thermo_.U(dynamic_op.m_dot_hot, dynamic_op.m_dot_cold, Rf_shell, Rf_tube, k_deposit);
  const double Atot = thermo_.geometry().areaOuter();
  const double dA = Atot / N;

  const double cp_h = thermo_.hot().cp;
  const double cp_c = thermo_.cold().cp;
  const double Ch = dynamic_op.m_dot_hot  * cp_h;  // hot-side capacity rate [W/K]
  const double Cc = dynamic_op.m_dot_cold * cp_c;  // cold-side capacity rate [W/K]

  const double Cth_h_cell = (cfg_.Mh / N) * cp_h;
  const double Cth_c_cell = (cfg_.Mc / N) * cp_c;

  // Bowman F correction applied uniformly to every cell so overall Q matches
  // the lumped model for shell-&-tube arrangements.
  double F = 1.0;
  if (cfg_.arrangement == FlowArrangement::ShellTube_1_2 ||
      cfg_.arrangement == FlowArrangement::ShellTube_2_4) {
    const double dT_in = dynamic_op.Tin_hot - dynamic_op.Tin_cold;
    if (dT_in > 1e-3) {
      const double dTc = std::max(state_.Tc_out - dynamic_op.Tin_cold, 1e-6);
      const double R   = (dynamic_op.Tin_hot - state_.Th_out) / dTc;
      const double P   = (state_.Tc_out - dynamic_op.Tin_cold) / dT_in;
      F = Thermo::lmtdCorrectionF(cfg_.arrangement, R, P);
    }
  }

  const bool counter = (cfg_.arrangement != FlowArrangement::ParallelFlow);

  // CFL-type sub-stepping: make the sub-step small enough that the fluid
  // advances at most half a cell per step even at the faster side.
  const double tau_h = (Ch > 1e-12) ? Cth_h_cell / Ch : dtMacro;
  const double tau_c = (Cc > 1e-12) ? Cth_c_cell / Cc : dtMacro;
  const double tau_min = std::max(1e-4, std::min(tau_h, tau_c));
  const int nSub = std::max(1, static_cast<int>(std::ceil(dtMacro / (0.5 * tau_min))));
  const double dtSub = dtMacro / nSub;

  std::vector<double> dTh(static_cast<size_t>(N), 0.0);
  std::vector<double> dTc(static_cast<size_t>(N), 0.0);
  double Q_total = 0.0;

  for (int sub = 0; sub < nSub; ++sub) {
    double Q_this = 0.0;

    for (int i = 0; i < N; ++i) {
      const double Th_i = Th_cell_[static_cast<size_t>(i)];
      const double Tc_i = Tc_cell_[static_cast<size_t>(i)];
      const double qi = Ut * dA * (Th_i - Tc_i) * F;
      Q_this += qi;

      // Upwind advection: hot fluid flows 0 → N-1.
      const double Th_up = (i == 0) ? dynamic_op.Tin_hot
                                    : Th_cell_[static_cast<size_t>(i - 1)];
      double Tc_up;
      if (counter) {
        Tc_up = (i == N - 1) ? dynamic_op.Tin_cold
                             : Tc_cell_[static_cast<size_t>(i + 1)];
      } else {
        Tc_up = (i == 0) ? dynamic_op.Tin_cold
                         : Tc_cell_[static_cast<size_t>(i - 1)];
      }

      dTh[static_cast<size_t>(i)] =
          (Ch * (Th_up - Th_i) - qi) / std::max(Cth_h_cell, 1e-12);
      dTc[static_cast<size_t>(i)] =
          (Cc * (Tc_up - Tc_i) + qi) / std::max(Cth_c_cell, 1e-12);
    }

    for (int i = 0; i < N; ++i) {
      Th_cell_[static_cast<size_t>(i)] += dTh[static_cast<size_t>(i)] * dtSub;
      Tc_cell_[static_cast<size_t>(i)] += dTc[static_cast<size_t>(i)] * dtSub;
      Th_cell_[static_cast<size_t>(i)] = std::clamp(Th_cell_[static_cast<size_t>(i)], 0.0, 200.0);
      Tc_cell_[static_cast<size_t>(i)] = std::clamp(Tc_cell_[static_cast<size_t>(i)], 0.0, 200.0);
    }

    Q_total = Q_this;
  }

  // Outlet scalars: hot outlet is always on the N-1 side; cold outlet depends
  // on flow direction (counter → x=0, parallel → x=L).
  state_.Th_out = Th_cell_[static_cast<size_t>(N - 1)];
  state_.Tc_out = counter ? Tc_cell_[0] : Tc_cell_[static_cast<size_t>(N - 1)];

  state_.U = Ut;
  state_.Q = std::max(0.0, std::min(1e6, Q_total));

  state_.Th_axial = Th_cell_;
  state_.Tc_axial = Tc_cell_;

  state_.dP_tube  = hydro_.dP_tube (dynamic_op.m_dot_hot,  Rf_tube,  k_deposit, thermo_.geometry().K_minor_tube);
  state_.dP_shell = hydro_.dP_shell(dynamic_op.m_dot_cold, Rf_shell, k_deposit, thermo_.geometry().K_turns_shell);
}

} // namespace hx
