#pragma once

#include "Simulator.hpp"
#include "Thermo.hpp"
#include "Hydraulics.hpp"
#include "Fouling.hpp"
#include "Types.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace hx {

/** \brief Monte-Carlo / sensitivity analysis settings.
 *
 *  Each trial perturbs a chosen subset of the plant inputs (flows, inlet
 *  temperatures, fluid properties, and fouling asymptote) by Gaussian
 *  noise with the σ values below.  Fractional σ multiplies the nominal
 *  value; absolute σ is in kelvin for the inlet temperatures.
 *
 *  The simulator is run in a clean, disturbance-free dynamic mode for
 *  \c trialSimTime seconds (lumped, 1-cell — fast enough to do hundreds
 *  of trials interactively).
 */
struct MonteCarloSettings {
  int      nTrials      = 200;      // total MC trial count
  double   trialSimTime = 60.0;     // [s] per-trial simulated horizon
  double   trialDt      = 0.5;      // [s] per-trial time step
  uint32_t seed         = 42;

  // Fractional σ (multiplied by nominal value)
  double frac_mhot    = 0.05;
  double frac_mcold   = 0.05;
  double frac_rho     = 0.02;
  double frac_mu      = 0.10;
  double frac_cp      = 0.02;
  double frac_k       = 0.05;
  double frac_RfMax   = 0.15;

  // Absolute σ [K] for inlet temperatures
  double abs_Tin_hot  = 1.0;
  double abs_Tin_cold = 0.5;

  bool includeFouling = true;       // also perturb RfMax & keep fouling on
};

struct MonteCarloStat {
  double mean   = 0.0;
  double stddev = 0.0;
  double p5     = 0.0;
  double p50    = 0.0;
  double p95    = 0.0;
  double minv   = 0.0;
  double maxv   = 0.0;
};

struct TornadoEntry {
  std::string name;           // human-readable parameter label
  double deltaPlus  = 0.0;    // Q(baseline + 1σ) - Q(baseline)  [W]
  double deltaMinus = 0.0;    // Q(baseline - 1σ) - Q(baseline)  [W]
  double sensitivity = 0.0;   // max(|deltaPlus|, |deltaMinus|)  [W]
};

struct MonteCarloResult {
  int                 nTrials = 0;
  std::vector<double> Q;         // [W]    per-trial steady heat duty
  std::vector<double> U;         // [W/m²K]
  std::vector<double> Tc_out;    // [°C]
  std::vector<double> Th_out;    // [°C]
  std::vector<double> dP_tube;   // [Pa]
  std::vector<double> dP_shell;  // [Pa]
  std::vector<double> eps;       // [-]   effectiveness
  std::vector<double> NTU;       // [-]

  MonteCarloStat statQ;
  MonteCarloStat statU;
  MonteCarloStat statTc;
  MonteCarloStat statEps;
  MonteCarloStat statDPt;
  MonteCarloStat statDPs;

  std::vector<TornadoEntry> tornado;  // sorted descending by sensitivity
  double baselineQ = 0.0;
  double baselineU = 0.0;

  bool ok = false;
  std::string message;
};

/** Progress callback: (current, total, phase). Return false to cancel. */
using MonteCarloProgress = std::function<bool(int, int, const char*)>;

/** \brief Execute nTrials + 2·K sensitivity trials of the digital twin.
 *
 *  The function is self-contained: it builds a throw-away Thermo /
 *  Hydraulics / Fouling / Simulator for each trial and does not touch
 *  any live simulator.  A baseline (nominal) trial is always run first.
 */
MonteCarloResult runMonteCarlo(const OperatingPoint &op0,
                               const Geometry       &geom,
                               const Fluid          &hot,
                               const Fluid          &cold,
                               const FoulingParams  &fp,
                               const SimConfig      &baseCfg,
                               const MonteCarloSettings &mc,
                               MonteCarloProgress   progress = {});

} // namespace hx
