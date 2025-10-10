#include "Config.hpp"

#include <fstream>
#include <stdexcept>

#include <toml++/toml.h>

namespace io {

static hx::FoulingParams::Model parseModel(const std::string &s) {
  if (s == "Asymptotic") return hx::FoulingParams::Model::Asymptotic;
  return hx::FoulingParams::Model::Linear;
}

AppConfig Config::fromToml(const std::string &path) {
  auto tbl = toml::parse_file(path);

  AppConfig c{};
  const auto &g = *tbl["geometry"].as_table();
  c.geometry.nTubes = (int)g["nTubes"].value_or(100);
  c.geometry.Di = g["Di"].value_or(0.019);
  c.geometry.Do = g["Do"].value_or(0.025);
  c.geometry.L = g["L"].value_or(5.0);
  c.geometry.pitch = g["pitch"].value_or(0.032);
  c.geometry.shellID = g["shellID"].value_or(0.5);
  c.geometry.baffleSpacing = g["baffleSpacing"].value_or(0.25);
  c.geometry.baffleCutFrac = g["baffleCutFrac"].value_or(0.25);
  c.geometry.nBaffles = (int)g["nBaffles"].value_or((int)(c.geometry.L / c.geometry.baffleSpacing));
  c.geometry.wall_k = g["wall_k"].value_or(16.0);
  c.geometry.wall_thickness = g["wall_thickness"].value_or(0.002);

  const auto &fh = *tbl["hot"].as_table();
  c.hot.rho = fh["rho"].value_or(997.0);
  c.hot.mu = fh["mu"].value_or(1e-3);
  c.hot.cp = fh["cp"].value_or(4180.0);
  c.hot.k = fh["k"].value_or(0.6);

  const auto &fc = *tbl["cold"].as_table();
  c.cold.rho = fc["rho"].value_or(997.0);
  c.cold.mu = fc["mu"].value_or(1e-3);
  c.cold.cp = fc["cp"].value_or(4180.0);
  c.cold.k = fc["k"].value_or(0.6);

  const auto &op = *tbl["operating"].as_table();
  c.op.m_dot_hot = op["m_dot_hot"].value_or(1.2);
  c.op.m_dot_cold = op["m_dot_cold"].value_or(1.0);
  c.op.Tin_hot = op["Tin_hot"].value_or(80.0);
  c.op.Tin_cold = op["Tin_cold"].value_or(25.0);

  const auto &f = *tbl["fouling"].as_table();
  c.fouling.Rf0 = f["Rf0"].value_or(0.0);
  c.fouling.RfMax = f["RfMax"].value_or(2e-4);
  c.fouling.tau = f["tau"].value_or(2e6);
  c.fouling.alpha = f["alpha"].value_or(0.0);
  c.fouling.model = parseModel(f["model"].value_or(std::string("Asymptotic")));

  const auto &lim = *tbl["limits"].as_table();
  c.limits.dP_tube_max = lim["dP_tube_max"].value_or(1e5);
  c.limits.dP_shell_max = lim["dP_shell_max"].value_or(1e5);
  c.limits.m_dot_cold_min = lim["m_dot_cold_min"].value_or(0.1);
  c.limits.m_dot_cold_max = lim["m_dot_cold_max"].value_or(5.0);

  return c;
}

} // namespace io
