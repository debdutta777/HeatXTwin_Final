#pragma once

#include "Types.hpp"

namespace hx {

/** \brief Thermo provides heat-transfer calculations (h, U, and steady ε–NTU mapping).
 */
class Thermo {
public:
  /** Construct with geometry and fluid properties. */
  Thermo(const Geometry &g, const Fluid &hot, const Fluid &cold);

  /** Tube-side convective coefficient using Dittus–Boelter (turbulent) or laminar fallback.
   *  @param m_dot_cold Cold-side mass flow [kg/s]
   *  @return h_t [W/m^2/K]
   */
  [[nodiscard]] double h_tube(double m_dot_cold) const;

  /** Shell-side convective coefficient using a Kern-style placeholder correlation.
   *  @param m_dot_hot Hot-side mass flow [kg/s]
   *  @return h_s [W/m^2/K]
   */
  [[nodiscard]] double h_shell(double m_dot_hot) const;

  /** Overall U based on outer area.
   *  1/U = 1/h_s + R_w + (1/h_t)*(Di/Do) + Rf_s + Rf_t
   */
  [[nodiscard]] double U(double m_dot_hot, double m_dot_cold, double Rf_shell, double Rf_tube) const;

  /** Steady-state mapping from inlets and flows to outlets and heat duty via ε–NTU.
   *  Counter-flow effectiveness is used by default.
   */
  [[nodiscard]] State steady(const OperatingPoint &op, double Rf_shell, double Rf_tube) const;

  /** Accessor methods for dynamic simulation */
  [[nodiscard]] const Geometry& geometry() const { return g_; }
  [[nodiscard]] const Fluid& hot() const { return hot_; }
  [[nodiscard]] const Fluid& cold() const { return cold_; }

private:
  Geometry g_;
  Fluid hot_;
  Fluid cold_;
};

} // namespace hx
