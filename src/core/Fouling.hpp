#pragma once

#include "Types.hpp"

namespace hx {

/** \brief Fouling provides thermal fouling growth models and optional thickness mapping. */
class Fouling {
public:
  explicit Fouling(const FoulingParams &p);
  [[nodiscard]] double Rf(double t_seconds) const;
  [[nodiscard]] double thickness(double Rf) const; // simple proportional mapping
  [[nodiscard]] const FoulingParams& params() const { return p_; }

private:
  FoulingParams p_;
};

} // namespace hx
