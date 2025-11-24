#pragma once

#include "Types.hpp"

namespace hx {

/** \brief Hydraulics provides pressure drop estimates for tube and shell sides. */
class Hydraulics {
public:
  Hydraulics(const Geometry &g, const Fluid &hot, const Fluid &cold);

  /** Tube-side pressure drop (Darcy–Weisbach + simple minor losses). */
  [[nodiscard]] double dP_tube(double m_dot_hot, double Rf_tube, double k_deposit, double K_minor) const;

  /** Shell-side pressure drop (approximate cross-flow model). */
  [[nodiscard]] double dP_shell(double m_dot_cold, double Rf_shell, double k_deposit, double K_turns) const;

private:
  Geometry g_;
  Fluid hot_;
  Fluid cold_;
};

} // namespace hx
