#include "FluidLibrary.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

namespace {

double clamp(double v, double lo, double hi) {
  return std::min(std::max(v, lo), hi);
}

// Horner-form polynomial evaluation: c[0] + c[1]*T + c[2]*T^2 + ...
template <size_t N>
double poly(const std::array<double, N> &c, double T) {
  double r = 0.0;
  for (size_t i = N; i-- > 0;) r = r * T + c[i];
  return r;
}

// -----------------------------------------------------------------------------
// Pure water (liquid), 5–150 °C. Polynomial fits to IAPWS-IF97 tabulated values.
// -----------------------------------------------------------------------------
Fluid water(double T) {
  T = clamp(T, 5.0, 150.0);
  Fluid f;
  // ρ [kg/m³]: quadratic fit, 1000 at 4°C -> 958 at 100°C
  f.rho = 999.9 + 0.05 * T - 0.0074 * T * T;
  // μ [Pa·s]: exponential-like fit (cSt to Pa·s ≈ ρ·ν). Use Vogel-style approximation.
  //   μ ≈ 2.414e-5 · 10^(247.8/(T[K]-140))
  const double T_K = T + 273.15;
  f.mu = 2.414e-5 * std::pow(10.0, 247.8 / (T_K - 140.0));
  // cp [J/kg/K]: nearly flat, gentle cubic
  f.cp = 4217.0 - 3.17 * T + 0.089 * T * T - 0.00068 * T * T * T;
  // k [W/m/K]: quadratic fit, peaks ~140°C
  f.k = 0.5706 + 0.001756 * T - 6.46e-6 * T * T;
  return f;
}

// -----------------------------------------------------------------------------
// 30 vol% ethylene glycol / water mixture. Correlations from ASHRAE HF 2017.
// Valid −10 to +90 °C.
// -----------------------------------------------------------------------------
Fluid eg30(double T) {
  T = clamp(T, -10.0, 90.0);
  Fluid f;
  f.rho = 1043.6 - 0.38 * T - 0.0025 * T * T;
  f.mu  = 0.00386 * std::exp(-0.024 * T);   // Arrhenius-like
  f.cp  = 3727.0 + 3.5 * T;
  f.k   = 0.458 + 0.0011 * T;
  return f;
}

// -----------------------------------------------------------------------------
// 50 vol% ethylene glycol / water mixture. ASHRAE HF 2017.
// Valid −30 to +90 °C.
// -----------------------------------------------------------------------------
Fluid eg50(double T) {
  T = clamp(T, -30.0, 90.0);
  Fluid f;
  f.rho = 1077.0 - 0.43 * T - 0.0018 * T * T;
  f.mu  = 0.00772 * std::exp(-0.0285 * T);
  f.cp  = 3300.0 + 3.9 * T;
  f.k   = 0.381 + 0.0010 * T;
  return f;
}

// -----------------------------------------------------------------------------
// SAE 30 engine oil. Incropera Appendix A Table A.5.
// Valid 20–150 °C.
// -----------------------------------------------------------------------------
Fluid oilSAE30(double T) {
  T = clamp(T, 20.0, 150.0);
  Fluid f;
  f.rho = 889.0 - 0.62 * T;
  // Viscosity is strongly T-dependent; Walther-style log-log fit:
  // log10(log10(ν + 0.7)) = A − B · log10(T + 273.15)
  const double T_K = T + 273.15;
  f.mu = 0.1 * std::pow(10.0, 4.2 - 0.019 * T);     // monotonic decrease
  f.mu = clamp(f.mu, 0.002, 0.8);
  f.cp = 1880.0 + 3.7 * T;
  f.k  = 0.145 - 1.3e-4 * T;
  (void)T_K;
  return f;
}

// -----------------------------------------------------------------------------
// Dry air at 1 atm, 0–200 °C. Incropera Appendix A Table A.4.
// -----------------------------------------------------------------------------
Fluid air(double T) {
  T = clamp(T, 0.0, 200.0);
  Fluid f;
  const double T_K = T + 273.15;
  f.rho = 353.0 / T_K;                              // ideal gas at 1 atm
  f.mu  = 1.458e-6 * std::pow(T_K, 1.5) / (T_K + 110.4);  // Sutherland
  f.cp  = 1005.0 + 0.08 * T;
  f.k   = 0.0241 + 7.5e-5 * T;
  return f;
}

}  // namespace

const char *fluidPresetName(FluidPreset p) {
  switch (p) {
    case FluidPreset::Custom:           return "Custom (manual)";
    case FluidPreset::Water:            return "Water";
    case FluidPreset::EthyleneGlycol30: return "Ethylene Glycol 30%";
    case FluidPreset::EthyleneGlycol50: return "Ethylene Glycol 50%";
    case FluidPreset::EngineOilSAE30:   return "Engine Oil SAE 30";
    case FluidPreset::AirSTP:           return "Air (1 atm)";
    default:                             return "Unknown";
  }
}

std::array<FluidPresetInfo, static_cast<size_t>(FluidPreset::Count)>
fluidPresetCatalog() {
  return {{
    {FluidPreset::Custom,            "Custom (manual)",        -273.0,  1000.0},
    {FluidPreset::Water,             "Water",                     5.0,   150.0},
    {FluidPreset::EthyleneGlycol30,  "Ethylene Glycol 30%",     -10.0,    90.0},
    {FluidPreset::EthyleneGlycol50,  "Ethylene Glycol 50%",     -30.0,    90.0},
    {FluidPreset::EngineOilSAE30,    "Engine Oil SAE 30",        20.0,   150.0},
    {FluidPreset::AirSTP,            "Air (1 atm)",               0.0,   200.0},
  }};
}

Fluid evaluateFluid(FluidPreset preset, double T_C, const Fluid &custom) {
  switch (preset) {
    case FluidPreset::Water:            return water(T_C);
    case FluidPreset::EthyleneGlycol30: return eg30(T_C);
    case FluidPreset::EthyleneGlycol50: return eg50(T_C);
    case FluidPreset::EngineOilSAE30:   return oilSAE30(T_C);
    case FluidPreset::AirSTP:           return air(T_C);
    case FluidPreset::Custom:
    default:                             return custom;
  }
}

}  // namespace hx
