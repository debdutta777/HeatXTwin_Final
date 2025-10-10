#pragma once

#include <cstdint>

namespace hx {

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
};

} // namespace hx
