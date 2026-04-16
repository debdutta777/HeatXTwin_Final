#include "AutoTune.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace hx {

AutotuneResult runRelayAutotune(const OperatingPoint &op0,
                                const Geometry &geom,
                                const Fluid &hot,
                                const Fluid &cold,
                                const FoulingParams &fp,
                                const SimConfig &baseCfg,
                                const AutotuneSettings &tune) {
  AutotuneResult out;

  // Build a throw-away simulator chain so the live Qt objects aren't
  // touched.  Force the internal PID off — the relay drives the MV.
  Thermo thermo(geom, hot, cold);
  Hydraulics hydro(geom, hot, cold);
  Fouling foul(fp);
  SimConfig cfg = baseCfg;
  thermo.setShellMethod(cfg.shellMethod);
  hydro .setShellMethod(cfg.shellMethod);
  cfg.pid.enabled = false;
  cfg.disturbanceType = SimConfig::DisturbanceType::None;  // no external noise
  cfg.numAxialCells = 1;  // lumped is faster and sufficient for frequency ID

  Simulator sim(thermo, hydro, foul, cfg);
  sim.setSteadyStateMode(false);    // keep dynamics, but no disturbances injected
  sim.setFoulingEnabled(false);     // clean plant identification

  OperatingPoint op = op0;
  op.m_dot_cold = tune.mv_bias;
  sim.reset(op);

  // The relay drives m_dot_cold = bias ± h.  Each time Tc_out crosses the
  // setpoint (with hysteresis) the relay flips.  Record the times of
  // zero-crossings to estimate Pu, and track the PV peaks to estimate
  // oscillation amplitude.
  const double h      = std::max(1e-4, tune.relay_h);
  const double hyst   = std::max(0.0, tune.hysteresis);
  const double sp     = tune.setpoint_Tc_out;
  const double dt     = std::max(1e-3, cfg.dt);
  const int    maxIt  = static_cast<int>(std::ceil(tune.maxSimTime / dt));

  bool relayHigh = true;                 // start ON so m_dot_cold rises
  std::vector<double> crossingTimes;     // setpoint crossings (for period)
  std::vector<double> crossingDirs;      // +1 = upward, -1 = downward
  double pvMax = -1e9, pvMin = 1e9;
  int    cyclesObserved = 0;
  double t = 0.0;

  // Skip the first N cells of transient — peaks before the plant rings up
  // typically don't represent the limit cycle.
  const int settleSteps = std::max(50, static_cast<int>(std::ceil(60.0 / dt)));

  for (int k = 0; k < maxIt; ++k) {
    // Relay law: flip when PV crosses setpoint ± hysteresis.
    op.m_dot_cold = tune.mv_bias + (relayHigh ? +h : -h);
    sim.updateOperatingPoint(op);

    const State &s = sim.step(t);
    const double y = s.Tc_out;

    if (k > settleSteps) {
      pvMax = std::max(pvMax, y);
      pvMin = std::min(pvMin, y);
    }

    // Detect a flip.  When relayHigh (MV high → cooling up → PV dropping
    // below sp − hyst), we flip to low.  And vice versa.
    if (relayHigh && y < sp - hyst) {
      relayHigh = false;
      if (k > settleSteps) { crossingTimes.push_back(t); crossingDirs.push_back(-1.0); }
    } else if (!relayHigh && y > sp + hyst) {
      relayHigh = true;
      if (k > settleSteps) { crossingTimes.push_back(t); crossingDirs.push_back(+1.0); }
    }

    // Count full periods: each upward crossing after the first defines one period.
    int upCrossings = 0;
    for (double d : crossingDirs) if (d > 0.0) ++upCrossings;
    cyclesObserved = std::max(0, upCrossings - 1);

    if (cyclesObserved >= tune.requiredCycles) break;
    t += dt;
  }

  out.cyclesObserved = cyclesObserved;
  if (cyclesObserved < 2) {
    out.ok = false;
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "Relay experiment did not produce a sustained oscillation "
                  "(%d cycles in %.0f s). Try a larger relay amplitude or "
                  "reduce hysteresis.", cyclesObserved, t);
    out.message = buf;
    return out;
  }

  // Extract upward-crossing time series, diff → period samples, average.
  std::vector<double> ups;
  for (size_t i = 0; i < crossingTimes.size(); ++i) {
    if (crossingDirs[i] > 0.0) ups.push_back(crossingTimes[i]);
  }
  std::vector<double> periods;
  for (size_t i = 1; i < ups.size(); ++i) periods.push_back(ups[i] - ups[i - 1]);
  if (periods.empty()) {
    out.ok = false;
    out.message = "Relay detected crossings but could not compute a period.";
    return out;
  }

  double Pu = 0.0;
  for (double p : periods) Pu += p;
  Pu /= static_cast<double>(periods.size());

  const double amplitude = std::max(1e-6, pvMax - pvMin);   // peak-to-peak [K]
  // The standard relay formula uses the half-amplitude a = (pvMax − pvMin)/2
  // because Ku = 4h / (π·a) assumes a is the 0-to-peak amplitude of the PV.
  const double a_half = 0.5 * amplitude;
  const double Ku = 4.0 * h / (M_PI * a_half);

  out.ok = true;
  out.Ku = Ku;
  out.Pu = Pu;
  // Ziegler-Nichols "classic PID" rule:
  //   Kp = 0.6·Ku,  Ti = 0.5·Pu,  Td = 0.125·Pu
  out.Kp = 0.6 * Ku;
  const double Ti = 0.5 * Pu;
  const double Td = 0.125 * Pu;
  out.Ki = (Ti > 1e-9) ? out.Kp / Ti : 0.0;
  out.Kd = out.Kp * Td;
  out.amplitude = amplitude;

  char buf[512];
  std::snprintf(buf, sizeof(buf),
                "Relay autotune converged: Ku=%.4g, Pu=%.2f s, amplitude=%.3f K over %d cycles. "
                "ZN classic rule → Kp=%.4g, Ki=%.4g, Kd=%.4g.",
                out.Ku, out.Pu, out.amplitude, out.cyclesObserved,
                out.Kp, out.Ki, out.Kd);
  out.message = buf;
  return out;
}

} // namespace hx
