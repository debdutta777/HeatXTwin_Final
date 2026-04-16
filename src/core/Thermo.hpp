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
   *  @param m_dot_hot Hot-side mass flow [kg/s] (Hot fluid is in tubes per report)
   *  @return h_t [W/m^2/K]
   */
  [[nodiscard]] double h_tube(double m_dot_hot) const;

  /** Shell-side convective coefficient using a Kern-style placeholder correlation.
   *  @param m_dot_cold Cold-side mass flow [kg/s] (Cold fluid is in shell per report)
   *  @return h_s [W/m^2/K]
   */
  [[nodiscard]] double h_shell(double m_dot_cold) const;

  /** Shell-side convective coefficient with effective diameter accounting for fouling.
   *  @param m_dot_cold Cold-side mass flow [kg/s]
   *  @param Rf_shell Shell-side fouling resistance [m²K/W]
   *  @param k_deposit Deposit thermal conductivity [W/m/K]
   *  @return h_s [W/m^2/K]
   */
  [[nodiscard]] double h_shell_with_fouling(double m_dot_cold, double Rf_shell, double k_deposit) const;

  /** Compute effective shell-side equivalent diameter with fouling deposit thickness.
   *  @param Rf_shell Shell-side fouling resistance [m²K/W]
   *  @param k_deposit Deposit thermal conductivity [W/m/K]
   *  @return De_eff [m]
   */
  [[nodiscard]] double De_effective(double Rf_shell, double k_deposit) const;

  /** Overall U based on outer area.
   *  1/U = 1/h_s + R_w + (1/h_t)*(Di/Do) + Rf_s + Rf_t
   */
  [[nodiscard]] double U(double m_dot_hot, double m_dot_cold, double Rf_shell, double Rf_tube, double k_deposit) const;

  /** Steady-state mapping from inlets and flows to outlets and heat duty via ε–NTU.
   *  \param arrangement Selects counter/parallel/shell-&-tube effectiveness formula.
   */
  [[nodiscard]] State steady(const OperatingPoint &op, double Rf_shell, double Rf_tube,
                               double k_deposit,
                               FlowArrangement arrangement = FlowArrangement::CounterFlow) const;

  /** LMTD correction factor F for the selected arrangement (dimensionless).
   *  Returns 1.0 for pure counter-flow. For shell-&-tube configurations uses
   *  the Bowman (1936) correlation based on P and R.
   *  \param R = (T_h,in − T_h,out) / (T_c,out − T_c,in)
   *  \param P = (T_c,out − T_c,in) / (T_h,in − T_c,in)
   */
  [[nodiscard]] static double lmtdCorrectionF(FlowArrangement arrangement, double R, double P);

  /** Accessor methods for dynamic simulation */
  [[nodiscard]] const Geometry& geometry() const { return g_; }
  [[nodiscard]] const Fluid& hot() const { return hot_; }
  [[nodiscard]] const Fluid& cold() const { return cold_; }

  /** Override fluid properties at runtime (e.g. temperature-dependent updates). */
  void setHot(const Fluid &f)  { hot_  = f; }
  void setCold(const Fluid &f) { cold_ = f; }

  /** Choose shell-side correlation: Kern (compact) or Bell–Delaware (segmented). */
  void setShellMethod(ShellSideMethod m) { shellMethod_ = m; }
  [[nodiscard]] ShellSideMethod shellMethod() const { return shellMethod_; }

private:
  Geometry g_;
  Fluid hot_;
  Fluid cold_;
  ShellSideMethod shellMethod_ = ShellSideMethod::Kern;
};

} // namespace hx
