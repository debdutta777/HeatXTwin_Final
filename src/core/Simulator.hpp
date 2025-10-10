#pragma once

#include "Model.hpp"

namespace hx {

struct SimConfig {
  double dt;    // [s]
  double tEnd;  // [s]
  int cells;    // for multi-cell later
  bool multicell;
  Limits limits;
};

/** \brief Simulator advances the model in time with simple first-order lags to emulate dynamics. */
class Simulator {
public:
  Simulator(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul, const SimConfig &cfg);

  void reset(const OperatingPoint &op0);
  [[nodiscard]] const State &step(double t);
  void updateOperatingPoint(const OperatingPoint &newOp) { op_ = newOp; }

  template <class F>
  void run(const OperatingPoint & /*schedule*/, F onSample) {
    double t = 0.0;
    while (t <= cfg_.tEnd) {
      [[maybe_unused]] const auto& currentState = step(t);
      if (!onSample(t, state_)) { // Allow callback to request early termination
        break;
      }
      t += cfg_.dt;
    }
  }

private:
  const Thermo &thermo_;
  const Hydraulics &hydro_;
  const Fouling &foul_;
  SimConfig cfg_;
  OperatingPoint op_{};
  State state_{};
};

} // namespace hx
