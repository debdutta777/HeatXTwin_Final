#pragma once

#include <cstdint>
#include <limits>
#include <vector>

namespace hx {

/** \brief Flow arrangement for the ε–NTU / LMTD calculation.
 *  Counter/Parallel are the limiting single-pass topologies; Shell-&-Tube
 *  presets implement the Bowman correction factor (F) for multi-pass TEMA
 *  configurations.
 */
enum class FlowArrangement : int {
  CounterFlow = 0,
  ParallelFlow,
  ShellTube_1_2,   // 1 shell pass, 2 (or more) tube passes
  ShellTube_2_4,   // 2 shell passes, 4 (or more) tube passes
};

/** \brief Which shell-side correlation the Thermo & Hydraulics components use.
 *  Kern (1950) is a compact, simple correlation suited for rough estimates.
 *  Bell–Delaware (1963) is the industry-standard segmented approach, breaking
 *  the shell flow into ideal crossflow modified by baffle-cut, leakage,
 *  bundle-bypass and laminar corrections.
 */
enum class ShellSideMethod : int {
  Kern         = 0,
  BellDelaware = 1,
};

struct Fluid {
  double rho; // density [kg/m^3]
  double mu;  // dynamic viscosity [Pa*s]
  double cp;  // specific heat [J/kg/K]
  double k;   // thermal conductivity [W/m/K]
};

struct Geometry {
  int nTubes;           // number of tubes
  double Di;            // inner diameter [m]
  double Do;            // outer diameter [m]
  double L;             // tube length [m]
  double pitch;         // tube pitch [m]
  double shellID;       // shell inner diameter [m]
  double baffleSpacing; // [m]
  double baffleCutFrac; // [-]
  int nBaffles;         // [-]
  double wall_k;        // wall conductivity [W/m/K]
  double wall_thickness;// wall thickness [m]
  
  // Hydraulic parameters
  double K_minor_tube = 1.5;      // Tube-side minor loss coefficient
  double K_turns_shell = 0.0;     // Shell-side turn loss coefficient (calculated if 0)

  [[nodiscard]] double areaOuter() const { return 3.14159265358979323846 * Do * L * nTubes; }
};

struct OperatingPoint {
  double m_dot_hot;  // [kg/s]
  double m_dot_cold; // [kg/s]
  double Tin_hot;    // [C]
  double Tin_cold;   // [C]
};

struct FoulingParams {
  double Rf0;     // initial fouling resistance [m^2*K/W]
  double RfMax;   // asymptotic max [m^2*K/W]
  double tau;     // time constant [s]
  double alpha;   // linear rate [m^2*K/(W*s)]
  double k_deposit = 0.5; // deposit thermal conductivity [W/m/K]
  double split_ratio = 0.5; // fraction of total fouling on shell side (0.0 to 1.0)
  enum class Model { Asymptotic, Linear } model;
};

struct Limits {
  double dP_tube_max;
  double dP_shell_max;
  double m_dot_cold_min;
  double m_dot_cold_max;
};

struct State {
  double Tc_out;    // [C]
  double Th_out;    // [C]
  double Q;         // [W]
  double U;         // [W/m^2/K]
  double Rf;        // [m^2*K/W]
  double dP_tube;   // [Pa]
  double dP_shell;  // [Pa]

  // Controller diagnostics — NaN when PID disabled
  double pidSetpoint = std::numeric_limits<double>::quiet_NaN();  // [C]  target Tc_out
  double pidColdFlow = std::numeric_limits<double>::quiet_NaN();  // [kg/s] commanded m_dot_cold (pre-actuator)

  // Advanced control diagnostics (NaN when inactive).
  //   pidFFterm          — feed-forward contribution to m_dot_cold command [kg/s]
  //   pidFBterm          — feedback (PID) contribution to m_dot_cold command [kg/s]
  //   pidColdFlowActual  — actuator-lag-filtered cold flow that actually reaches the exchanger [kg/s]
  double pidFFterm         = std::numeric_limits<double>::quiet_NaN();
  double pidFBterm         = std::numeric_limits<double>::quiet_NaN();
  double pidColdFlowActual = std::numeric_limits<double>::quiet_NaN();

  // Axial temperature profile (finite-volume discretization).
  // Both vectors share cell indexing 0..N-1 where x=0 is the HOT inlet side
  // and x=L is the HOT outlet side (i.e. the flow direction of the hot fluid).
  // Empty when the simulator runs in lumped mode (numAxialCells <= 1).
  std::vector<double> Th_axial;   // hot-side temperature at each cell [C]
  std::vector<double> Tc_axial;   // cold-side temperature at each cell [C]
};

} // namespace hx
