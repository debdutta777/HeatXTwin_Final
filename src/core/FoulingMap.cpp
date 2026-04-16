#include "FoulingMap.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hx {

namespace {

constexpr double PI = 3.14159265358979323846;

/**
 *  Generate triangular-pitch tube centres inside a circle of radius
 *  \c R_bundle, returning at most \c nTubes positions (picking the innermost
 *  ones first so real bundles always "fill from centre out").
 */
std::vector<TubePos> layoutTriangularBundle(int nTubes, double pitch, double R_bundle) {
  std::vector<TubePos> out;
  out.reserve(static_cast<size_t>(std::max(0, nTubes)));
  if (nTubes <= 0 || pitch <= 0.0 || R_bundle <= 0.0) return out;

  // Enumerate a generous grid and keep only cells inside the circle.
  const double dy = pitch * std::sqrt(3.0) / 2.0;
  const int    k  = static_cast<int>(std::ceil(R_bundle / pitch)) + 2;

  struct Candidate { double x, y, r; };
  std::vector<Candidate> pool;
  pool.reserve(static_cast<size_t>((2 * k + 1) * (2 * k + 1)));

  for (int j = -k; j <= k; ++j) {
    const double yy = j * dy;
    const double xoff = (j & 1) ? 0.5 * pitch : 0.0;
    for (int i = -k; i <= k; ++i) {
      const double xx = i * pitch + xoff;
      const double r  = std::hypot(xx, yy);
      if (r <= R_bundle) pool.push_back({xx, yy, r});
    }
  }
  // Closest-to-centre first so nTubes truncation yields a realistic packing.
  std::sort(pool.begin(), pool.end(),
            [](const Candidate &a, const Candidate &b) { return a.r < b.r; });

  const int take = std::min(nTubes, static_cast<int>(pool.size()));
  out.reserve(static_cast<size_t>(take));
  for (int idx = 0; idx < take; ++idx) {
    TubePos t;
    t.id     = idx;
    t.x      = pool[static_cast<size_t>(idx)].x;
    t.y      = pool[static_cast<size_t>(idx)].y;
    t.r_norm = (R_bundle > 1e-9) ? pool[static_cast<size_t>(idx)].r / R_bundle : 0.0;
    out.push_back(t);
  }
  return out;
}

} // anonymous namespace

FoulingMap computeFoulingMap(const Geometry       &g,
                              const OperatingPoint &op,
                              double                Rf_bulk,
                              int                   nAxial) {
  FoulingMap m;
  m.nAxial = std::max(2, nAxial);

  const int    nT       = std::max(1, g.nTubes);
  const double pitch    = std::max(1e-6, g.pitch);
  const double Rbundle  = std::max(0.5 * g.shellID - 0.5 * g.Do, 1e-6);
  m.tubes = layoutTriangularBundle(nT, pitch, Rbundle);

  const int N = m.nAxial;
  m.Rf.assign(m.tubes.size(), std::vector<double>(static_cast<size_t>(N), 0.0));
  m.Rf_mean .assign(m.tubes.size(), 0.0);
  m.Rf_axial.assign(static_cast<size_t>(N), 0.0);

  if (Rf_bulk <= 0.0 || m.tubes.empty()) {
    m.Rf_min = m.Rf_max = m.Rf_mean_overall = 0.0;
    return m;
  }

  // --- Physics model -------------------------------------------------------
  // 1. Radial bypass bias: fouling factor peaks at centre (r=0), decays
  //    smoothly to a minimum at the shell wall (r=1).  "cosine-hump" profile:
  //       f_radial(r) = 1 + A_r · (1 − r²)
  //    with A_r = 0.8  (centre tubes foul ~80 % more than wall tubes)
  const double A_r = 0.8;

  // 2. Baffle-cut stagnation: tubes in the window (|y| > (1 − baffleCut) · R)
  //    sit in recirculation eddies → enhanced deposition.
  //    f_win(y) = 1 + A_w · σ((|y|/R − (1 − bc)) · 10)
  //    with σ = logistic, A_w = 0.25, bc = baffle cut fraction.
  const double bc  = std::clamp(g.baffleCutFrac, 0.15, 0.45);
  const double A_w = 0.25;

  // 3. Axial build-up:   f_axial(ζ) = 1 + γ · ζ         ζ ∈ [0,1]
  //    γ ≈ 0.6 gives the downstream cells ~60 % more fouling than the inlet.
  const double gamma = 0.6;

  // 4. Operating-point shear modifier: higher shell-side flow shears film.
  //    We use op.m_dot_cold as a proxy for shell-side shear (cold is the
  //    shell-side stream in this simulator by convention).  Normalised so
  //    m_dot_cold = 2 kg/s → shear factor = 1.
  const double shear = std::clamp(std::sqrt(2.0 / std::max(op.m_dot_cold, 1e-3)),
                                   0.5, 2.0);

  // Build raw (un-normalised) field.
  // Outer-surface area is identical for every tube, so we track it as a
  // scalar; the area-weighted mean reduces to the arithmetic mean but we
  // keep the explicit weighting to preserve the physical intent.
  const double tubeAreaEach = PI * g.Do * g.L;
  const double thresh       = 1.0 - bc;                   // constant per run
  const double invRbundle   = 1.0 / std::max(Rbundle, 1e-9);
  const double invNm1       = (N > 1) ? 1.0 / (N - 1) : 0.0;
  const double invN         = 1.0 / static_cast<double>(N);
  double weighted_sum = 0.0;
  double area_sum     = 0.0;

  for (size_t i = 0; i < m.tubes.size(); ++i) {
    const TubePos &t = m.tubes[i];

    const double f_rad = 1.0 + A_r * (1.0 - t.r_norm * t.r_norm);
    const double rel_y = std::fabs(t.y) * invRbundle;
    const double logistic = 1.0 / (1.0 + std::exp(-10.0 * (rel_y - thresh)));
    const double f_win = 1.0 + A_w * logistic;

    // All radial/baffle-cut/shear factors are constant within the j loop.
    const double tubeBase = Rf_bulk * f_rad * f_win * shear;

    auto &row = m.Rf[i];
    double sum_ax = 0.0;
    for (int j = 0; j < N; ++j) {
      const double zeta = (N == 1) ? 0.5 : static_cast<double>(j) * invNm1;
      const double raw  = tubeBase * (1.0 + gamma * zeta);
      row[static_cast<size_t>(j)] = raw;
      sum_ax += raw;
    }

    const double tube_mean = sum_ax * invN;
    m.Rf_mean[i] = tube_mean;
    weighted_sum += tube_mean * tubeAreaEach;
    area_sum     += tubeAreaEach;
  }

  // --- Renormalise so area-weighted mean exactly equals Rf_bulk ------------
  const double raw_mean = (area_sum > 0.0) ? weighted_sum / area_sum : Rf_bulk;
  const double scale    = (raw_mean > 1e-15) ? (Rf_bulk / raw_mean) : 1.0;

  m.Rf_min = std::numeric_limits<double>::infinity();
  m.Rf_max = 0.0;
  m.Rf_mean_overall = 0.0;
  std::vector<double> axial_accum(static_cast<size_t>(N), 0.0);

  int hotCount = 0;
  const double hotThreshold = 1.5 * Rf_bulk;

  for (size_t i = 0; i < m.tubes.size(); ++i) {
    auto &row = m.Rf[i];
    double tube_sum = 0.0;
    for (int j = 0; j < N; ++j) {
      const size_t jz = static_cast<size_t>(j);
      const double v = row[jz] * scale;
      row[jz] = v;
      tube_sum += v;
      axial_accum[jz] += v * tubeAreaEach;
      if (v < m.Rf_min) m.Rf_min = v;
      if (v > m.Rf_max) m.Rf_max = v;
    }
    m.Rf_mean[i] = tube_sum * invN;
    if (m.Rf_mean[i] > hotThreshold) ++hotCount;
  }
  m.Rf_mean_overall = Rf_bulk;  // by construction after normalisation
  m.hot_spot_fraction = (m.tubes.empty()) ? 0.0
                         : static_cast<double>(hotCount) / static_cast<double>(m.tubes.size());

  for (int j = 0; j < N; ++j) {
    m.Rf_axial[static_cast<size_t>(j)] = (area_sum > 0.0)
        ? axial_accum[static_cast<size_t>(j)] / area_sum
        : 0.0;
  }

  if (!std::isfinite(m.Rf_min)) m.Rf_min = 0.0;
  return m;
}

} // namespace hx
