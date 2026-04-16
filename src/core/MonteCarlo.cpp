#include "MonteCarlo.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace hx {

namespace {

/** Run one trial and return the simulator's final State. */
State runOneTrial(const OperatingPoint &op,
                  const Geometry       &geom,
                  const Fluid          &hot,
                  const Fluid          &cold,
                  const FoulingParams  &fp,
                  const SimConfig      &baseCfg,
                  double                trialSimTime,
                  double                trialDt,
                  bool                  foulingEnabled) {
  SimConfig cfg = baseCfg;
  cfg.dt              = trialDt;
  cfg.tEnd            = trialSimTime;
  cfg.pid.enabled     = false;                       // exclude control dynamics
  cfg.disturbanceType = SimConfig::DisturbanceType::None;
  cfg.numAxialCells   = 1;                           // lumped → fast
  cfg.scenario.clear();                              // no scripted timeline

  Thermo     thermo(geom, hot, cold);
  Hydraulics hydro (geom, hot, cold);
  Fouling    foul  (fp);
  thermo.setShellMethod(cfg.shellMethod);
  hydro .setShellMethod(cfg.shellMethod);

  Simulator sim(thermo, hydro, foul, cfg);
  sim.setSteadyStateMode(false);     // allow dynamics so it settles
  sim.setFoulingEnabled(foulingEnabled);
  sim.reset(op);

  State last{};
  const int nSteps = static_cast<int>(std::ceil(trialSimTime / trialDt));
  double t = 0.0;
  for (int k = 0; k < nSteps; ++k) {
    last = sim.step(t);
    t += trialDt;
  }
  return last;
}

MonteCarloStat computeStats(std::vector<double> v) {
  MonteCarloStat s{};
  if (v.empty()) return s;
  double sum = 0.0;
  for (double x : v) sum += x;
  s.mean = sum / static_cast<double>(v.size());
  double sq = 0.0;
  for (double x : v) sq += (x - s.mean) * (x - s.mean);
  s.stddev = (v.size() > 1) ? std::sqrt(sq / static_cast<double>(v.size() - 1)) : 0.0;

  std::sort(v.begin(), v.end());
  s.minv = v.front();
  s.maxv = v.back();
  auto pct = [&](double p) {
    if (v.size() == 1) return v.front();
    const double idx = p * static_cast<double>(v.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    const double frac = idx - static_cast<double>(lo);
    return v[lo] * (1.0 - frac) + v[hi] * frac;
  };
  s.p5  = pct(0.05);
  s.p50 = pct(0.50);
  s.p95 = pct(0.95);
  return s;
}

double computeEpsilon(const OperatingPoint &op, const Fluid &hot, const Fluid &cold,
                      const State &s) {
  const double Ch   = op.m_dot_hot  * hot.cp;
  const double Cc   = op.m_dot_cold * cold.cp;
  const double Cmin = std::min(Ch, Cc);
  const double Qmax = (Cmin > 0.0) ? Cmin * (op.Tin_hot - op.Tin_cold) : 0.0;
  return (Qmax > 1e-9) ? s.Q / Qmax : 0.0;
}

double computeNTU(const OperatingPoint &op, const Fluid &hot, const Fluid &cold,
                  const Geometry &geom, const State &s) {
  const double Ch   = op.m_dot_hot  * hot.cp;
  const double Cc   = op.m_dot_cold * cold.cp;
  const double Cmin = std::min(Ch, Cc);
  return (Cmin > 0.0 && s.U > 0.0) ? (s.U * geom.areaOuter()) / Cmin : 0.0;
}

} // anonymous namespace

MonteCarloResult runMonteCarlo(const OperatingPoint &op0,
                               const Geometry       &geom,
                               const Fluid          &hot,
                               const Fluid          &cold,
                               const FoulingParams  &fp,
                               const SimConfig      &baseCfg,
                               const MonteCarloSettings &mc,
                               MonteCarloProgress   progress) {
  MonteCarloResult out;
  if (mc.nTrials < 2) {
    out.ok = false;
    out.message = "Monte-Carlo needs at least 2 trials.";
    return out;
  }

  // Baseline (nominal) trial.
  const State sBase = runOneTrial(op0, geom, hot, cold, fp, baseCfg,
                                  mc.trialSimTime, mc.trialDt, mc.includeFouling);
  out.baselineQ = sBase.Q;
  out.baselineU = sBase.U;

  out.Q       .reserve(static_cast<size_t>(mc.nTrials));
  out.U       .reserve(static_cast<size_t>(mc.nTrials));
  out.Tc_out  .reserve(static_cast<size_t>(mc.nTrials));
  out.Th_out  .reserve(static_cast<size_t>(mc.nTrials));
  out.dP_tube .reserve(static_cast<size_t>(mc.nTrials));
  out.dP_shell.reserve(static_cast<size_t>(mc.nTrials));
  out.eps     .reserve(static_cast<size_t>(mc.nTrials));
  out.NTU     .reserve(static_cast<size_t>(mc.nTrials));

  std::mt19937 rng(mc.seed);
  std::normal_distribution<double> nd(0.0, 1.0);

  const int totalWork = mc.nTrials + 22;  // 2 trials per tornado parameter (~11)
  int done = 0;
  if (progress && !progress(0, totalWork, "Running Monte-Carlo trials")) {
    out.ok = false;
    out.message = "Cancelled.";
    return out;
  }

  // === Full Monte-Carlo ensemble: perturb everything ===
  for (int k = 0; k < mc.nTrials; ++k) {
    OperatingPoint op = op0;
    Fluid h = hot;
    Fluid c = cold;
    FoulingParams fpk = fp;

    op.m_dot_hot  *= std::max(0.1, 1.0 + mc.frac_mhot  * nd(rng));
    op.m_dot_cold *= std::max(0.1, 1.0 + mc.frac_mcold * nd(rng));
    op.Tin_hot   += mc.abs_Tin_hot  * nd(rng);
    op.Tin_cold  += mc.abs_Tin_cold * nd(rng);

    h.rho *= std::max(0.1, 1.0 + mc.frac_rho * nd(rng));
    h.mu  *= std::max(0.1, 1.0 + mc.frac_mu  * nd(rng));
    h.cp  *= std::max(0.1, 1.0 + mc.frac_cp  * nd(rng));
    h.k   *= std::max(0.1, 1.0 + mc.frac_k   * nd(rng));

    c.rho *= std::max(0.1, 1.0 + mc.frac_rho * nd(rng));
    c.mu  *= std::max(0.1, 1.0 + mc.frac_mu  * nd(rng));
    c.cp  *= std::max(0.1, 1.0 + mc.frac_cp  * nd(rng));
    c.k   *= std::max(0.1, 1.0 + mc.frac_k   * nd(rng));

    if (mc.includeFouling) {
      fpk.RfMax *= std::max(0.1, 1.0 + mc.frac_RfMax * nd(rng));
    }

    const State s = runOneTrial(op, geom, h, c, fpk, baseCfg,
                                mc.trialSimTime, mc.trialDt, mc.includeFouling);

    out.Q       .push_back(s.Q);
    out.U       .push_back(s.U);
    out.Tc_out  .push_back(s.Tc_out);
    out.Th_out  .push_back(s.Th_out);
    out.dP_tube .push_back(s.dP_tube);
    out.dP_shell.push_back(s.dP_shell);
    out.eps     .push_back(computeEpsilon(op, h, c, s));
    out.NTU     .push_back(computeNTU    (op, h, c, geom, s));

    ++done;
    if (progress && (k % 5 == 0 || k == mc.nTrials - 1)) {
      if (!progress(done, totalWork, "Running Monte-Carlo trials")) {
        out.ok = false;
        out.message = "Cancelled.";
        return out;
      }
    }
  }
  out.nTrials = static_cast<int>(out.Q.size());

  // === Tornado analysis: ±1σ per parameter, all others nominal ===
  struct ParamPerturb {
    const char *name;
    // delta applied in place; returns perturbed copies of op/hot/cold/fp
    std::function<void(OperatingPoint&, Fluid&, Fluid&, FoulingParams&, double /*sign*/)> apply;
  };

  std::vector<ParamPerturb> params = {
    {"m\u0307 hot",   [&](OperatingPoint &o, Fluid&, Fluid&, FoulingParams&, double s) {
        o.m_dot_hot  *= std::max(0.1, 1.0 + s * mc.frac_mhot);
    }},
    {"m\u0307 cold",  [&](OperatingPoint &o, Fluid&, Fluid&, FoulingParams&, double s) {
        o.m_dot_cold *= std::max(0.1, 1.0 + s * mc.frac_mcold);
    }},
    {"T in, hot",    [&](OperatingPoint &o, Fluid&, Fluid&, FoulingParams&, double s) {
        o.Tin_hot   += s * mc.abs_Tin_hot;
    }},
    {"T in, cold",   [&](OperatingPoint &o, Fluid&, Fluid&, FoulingParams&, double s) {
        o.Tin_cold  += s * mc.abs_Tin_cold;
    }},
    {"\u03C1 hot",   [&](OperatingPoint&, Fluid &h, Fluid&, FoulingParams&, double s) {
        h.rho *= std::max(0.1, 1.0 + s * mc.frac_rho);
    }},
    {"\u03BC hot",   [&](OperatingPoint&, Fluid &h, Fluid&, FoulingParams&, double s) {
        h.mu  *= std::max(0.1, 1.0 + s * mc.frac_mu);
    }},
    {"c\u209A hot",  [&](OperatingPoint&, Fluid &h, Fluid&, FoulingParams&, double s) {
        h.cp  *= std::max(0.1, 1.0 + s * mc.frac_cp);
    }},
    {"k hot",        [&](OperatingPoint&, Fluid &h, Fluid&, FoulingParams&, double s) {
        h.k   *= std::max(0.1, 1.0 + s * mc.frac_k);
    }},
    {"\u03C1 cold",  [&](OperatingPoint&, Fluid&, Fluid &c, FoulingParams&, double s) {
        c.rho *= std::max(0.1, 1.0 + s * mc.frac_rho);
    }},
    {"\u03BC cold",  [&](OperatingPoint&, Fluid&, Fluid &c, FoulingParams&, double s) {
        c.mu  *= std::max(0.1, 1.0 + s * mc.frac_mu);
    }},
    {"c\u209A cold", [&](OperatingPoint&, Fluid&, Fluid &c, FoulingParams&, double s) {
        c.cp  *= std::max(0.1, 1.0 + s * mc.frac_cp);
    }},
    {"k cold",       [&](OperatingPoint&, Fluid&, Fluid &c, FoulingParams&, double s) {
        c.k   *= std::max(0.1, 1.0 + s * mc.frac_k);
    }},
  };
  if (mc.includeFouling) {
    params.push_back({"R\u1D9C f\u2099\u2098\u2090\u2093",
      [&](OperatingPoint&, Fluid&, Fluid&, FoulingParams &f, double s) {
        f.RfMax *= std::max(0.1, 1.0 + s * mc.frac_RfMax);
    }});
  }

  out.tornado.reserve(params.size());
  if (progress && !progress(done, totalWork, "Tornado sensitivity")) {
    out.ok = false;
    out.message = "Cancelled.";
    return out;
  }

  for (const auto &p : params) {
    OperatingPoint op_p = op0;  Fluid h_p = hot; Fluid c_p = cold; FoulingParams fp_p = fp;
    OperatingPoint op_m = op0;  Fluid h_m = hot; Fluid c_m = cold; FoulingParams fp_m = fp;
    p.apply(op_p, h_p, c_p, fp_p, +1.0);
    p.apply(op_m, h_m, c_m, fp_m, -1.0);

    const State sPlus  = runOneTrial(op_p, geom, h_p, c_p, fp_p, baseCfg,
                                     mc.trialSimTime, mc.trialDt, mc.includeFouling);
    const State sMinus = runOneTrial(op_m, geom, h_m, c_m, fp_m, baseCfg,
                                     mc.trialSimTime, mc.trialDt, mc.includeFouling);

    TornadoEntry e;
    e.name        = p.name;
    e.deltaPlus   = sPlus.Q  - out.baselineQ;
    e.deltaMinus  = sMinus.Q - out.baselineQ;
    e.sensitivity = std::max(std::fabs(e.deltaPlus), std::fabs(e.deltaMinus));
    out.tornado.push_back(e);

    done += 2;
    if (progress && !progress(done, totalWork, "Tornado sensitivity")) {
      out.ok = false;
      out.message = "Cancelled.";
      return out;
    }
  }

  // Sort tornado descending by |sensitivity|
  std::sort(out.tornado.begin(), out.tornado.end(),
            [](const TornadoEntry &a, const TornadoEntry &b) {
              return a.sensitivity > b.sensitivity;
            });

  out.statQ   = computeStats(out.Q);
  out.statU   = computeStats(out.U);
  out.statTc  = computeStats(out.Tc_out);
  out.statEps = computeStats(out.eps);
  out.statDPt = computeStats(out.dP_tube);
  out.statDPs = computeStats(out.dP_shell);

  out.ok = true;
  char buf[256];
  std::snprintf(buf, sizeof(buf),
                "Monte-Carlo complete: %d trials, %zu tornado params. "
                "Q = %.1f ± %.1f W (p5..p95: %.1f..%.1f).",
                out.nTrials, out.tornado.size(),
                out.statQ.mean, out.statQ.stddev, out.statQ.p5, out.statQ.p95);
  out.message = buf;
  return out;
}

} // namespace hx
