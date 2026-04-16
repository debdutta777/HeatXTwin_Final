#pragma once

#include "Simulator.hpp"
#include "Thermo.hpp"
#include "Hydraulics.hpp"
#include "Fouling.hpp"
#include "Types.hpp"
#include <string>

namespace hx {

/**
 * \brief Åström-Hägglund relay autotune.
 *
 * Runs an offline closed-loop experiment where the MV (cold mass flow)
 * is driven by a bang-bang relay around the bias value.  When a
 * self-sustained oscillation is established, the peak-to-peak amplitude
 * \c a of the PV (Tc_out) and its period \c Pu give the ultimate
 * parameters of the plant:
 *
 *      K_u = 4·h / (π · a)
 *      P_u = average peak-to-peak period
 *
 * Ziegler-Nichols "classic PID" rule then yields recommended gains:
 *
 *      K_p = 0.6 · K_u
 *      T_i = 0.5 · P_u     → K_i = K_p / T_i
 *      T_d = 0.125 · P_u   → K_d = K_p · T_d
 *
 * The simulation runs at wall-clock-independent speed inside this
 * function; the caller blocks until the experiment finishes (a few ms
 * to a second depending on dt / simulated horizon).
 */
struct AutotuneSettings {
  double setpoint_Tc_out = 45.0;   // [C] bias around which the relay swings
  double relay_h         = 0.5;    // [kg/s] ±h bang-bang amplitude on m_dot_cold
  double mv_bias         = 1.0;    // [kg/s] center cold-flow operating point
  double hysteresis      = 0.1;    // [C]  dead-band on PV to reject noise
  int    requiredCycles  = 5;      // # full periods required before accepting
  double maxSimTime      = 2000.0; // [s]   hard cap on experiment duration
};

struct AutotuneResult {
  bool ok = false;
  double Ku = 0.0;   // ultimate gain      [kg/s / K]
  double Pu = 0.0;   // ultimate period    [s]
  double Kp = 0.0;
  double Ki = 0.0;
  double Kd = 0.0;
  double amplitude = 0.0;   // peak-to-peak PV swing [K]
  int    cyclesObserved = 0;
  std::string message;      // human-readable status
};

/**
 * Runs the relay experiment using a self-contained Simulator instance so
 * that it does not disturb any live simulator the UI is already running.
 *
 * \note The simulator PID loop is forced off for the duration of the
 *       experiment — the autotune drives the MV directly via the relay.
 */
AutotuneResult runRelayAutotune(const OperatingPoint &op0,
                                const Geometry &geom,
                                const Fluid &hot,
                                const Fluid &cold,
                                const FoulingParams &fp,
                                const SimConfig &baseCfg,
                                const AutotuneSettings &tune);

} // namespace hx
