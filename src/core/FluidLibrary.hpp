#pragma once

#include "Types.hpp"
#include <array>
#include <string>

namespace hx {

/**
 * \brief Temperature-dependent fluid property library.
 *
 * Each preset provides polynomial fits to ρ, μ, cₚ, k as functions of
 * temperature (°C). Correlations are sourced from standard engineering
 * references (Incropera/DeWitt, VDI Heat Atlas, ASHRAE Handbook) and are
 * valid across the stated temperature range. Outside the range the
 * coefficients are clamped to the endpoint value to keep the simulator stable.
 */
enum class FluidPreset : int {
  Custom = 0,            // user-defined (spinboxes drive properties)
  Water,                 // pure water, 5–150 °C
  EthyleneGlycol30,      // 30 vol% EG / water, −10–90 °C
  EthyleneGlycol50,      // 50 vol% EG / water, −30–90 °C
  EngineOilSAE30,        // SAE 30 motor oil, 20–150 °C
  AirSTP,                // dry air, 0–200 °C
  Count
};

struct FluidPresetInfo {
  FluidPreset preset;
  const char *displayName;
  double T_min;   // [°C] recommended lower bound
  double T_max;   // [°C] recommended upper bound
};

/** Human-readable display name (use in UI combo boxes). */
const char *fluidPresetName(FluidPreset p);

/** Iterable catalogue of available presets with operating ranges. */
std::array<FluidPresetInfo, static_cast<size_t>(FluidPreset::Count)> fluidPresetCatalog();

/**
 * \brief Evaluate ρ, μ, cₚ, k at temperature T_C (°C) for a given preset.
 * For FluidPreset::Custom the caller's `custom` value is returned unchanged.
 */
Fluid evaluateFluid(FluidPreset preset, double T_C, const Fluid &custom = {});

}  // namespace hx
