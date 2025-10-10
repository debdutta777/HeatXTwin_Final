#pragma once

#include <string>

#include "core/Types.hpp"

namespace io {

struct AppConfig {
  hx::Geometry geometry;
  hx::Fluid hot;
  hx::Fluid cold;
  hx::OperatingPoint op;
  hx::FoulingParams fouling;
  hx::Limits limits;
};

class Config {
public:
  static AppConfig fromToml(const std::string &path);
};

} // namespace io
