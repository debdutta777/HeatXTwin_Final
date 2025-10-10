#pragma once

#include "Fouling.hpp"
#include "Hydraulics.hpp"
#include "Thermo.hpp"

namespace hx {

/** \brief Model ties Thermo and Hydraulics together for a single time step. */
class Model {
public:
  Model(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul);
  [[nodiscard]] State evaluate(const OperatingPoint &op, double t_seconds) const;

private:
  const Thermo &thermo_;
  const Hydraulics &hydro_;
  const Fouling &foul_;
};

} // namespace hx
