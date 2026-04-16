#pragma once

#include "Types.hpp"
#include <vector>

namespace hx {

/**
 *  Spatial location of a single tube inside the shell (cross-section plane).
 *  Cells are laid out on an equilateral-triangular pitch and truncated to the
 *  shell inner radius minus one tube radius.  Index \c id is 0-based and is
 *  stable across calls for the same \c Geometry.
 */
struct TubePos {
  int    id;           // 0-based tube index (global)
  double x;            // [m] centre-x in bundle plane (0 at shell axis)
  double y;            // [m] centre-y
  double r_norm;       // [0,1] radial position / shell-radius (centre=0, wall=1)
};

/**
 *  Per-tube fouling "heatmap" — fouling resistance Rf for every tube and axial
 *  cell, plus derived summary stats.  Produced by \c computeFoulingMap() from
 *  the bulk fouling state \c Rf_bulk and the current operating point.
 */
struct FoulingMap {
  std::vector<TubePos>            tubes;     // layout (size = nTubes)
  std::vector<std::vector<double>> Rf;       // Rf[iTube][iAxial]  [m²·K/W]
  std::vector<double>             Rf_mean;   // axial-averaged Rf per tube [m²·K/W]
  std::vector<double>             Rf_axial;  // area-weighted Rf vs axial cell
  int                             nAxial = 20;

  // Summary
  double Rf_min   = 0.0;
  double Rf_max   = 0.0;
  double Rf_mean_overall = 0.0;  // area-weighted global mean — should equal Rf_bulk
  double hot_spot_fraction = 0.0; // fraction of tubes with Rf_mean > 1.5·Rf_bulk
};

/**
 *  Compute a physically-motivated per-tube / per-axial-cell fouling
 *  distribution around the given bulk fouling resistance.
 *
 *  The model superposes four physical effects:
 *    1. **Radial bypass bias** — tubes near the shell wall see crossflow
 *       bypass (J_b in Bell-Delaware) and therefore higher shear, which
 *       reduces local deposition.  Tubes near the centre of the bundle
 *       experience lower shear → enhanced deposition.
 *    2. **Baffle-cut stagnation** — tubes in the baffle "window" (±baffleCut
 *       from the shell axis in the y-direction) have recirculation zones that
 *       collect particulates and film more aggressively.
 *    3. **Axial build-up** — along the flow direction, particulates deposit
 *       progressively so downstream axial cells foul faster:
 *            Rf(z)  ∝  1 + γ · z/L        (γ ≈ 0.6, tunable)
 *    4. **Local velocity perturbation** — operating-point flow adjusts the
 *       absolute depth of fouling (higher flow → higher shear → thinner film).
 *
 *  All per-tube values are then renormalised so the area-weighted global mean
 *  recovers the bulk \c Rf_bulk exactly (conservation of mass of deposit).
 *
 *  \param g         heat-exchanger geometry
 *  \param op        current operating point (drives flow-shear modifier)
 *  \param Rf_bulk   lumped fouling resistance from the integrator [m²·K/W]
 *  \param nAxial    number of axial cells to discretise each tube into
 */
FoulingMap computeFoulingMap(const Geometry       &g,
                              const OperatingPoint &op,
                              double                Rf_bulk,
                              int                   nAxial = 20);

} // namespace hx
