#include "Model.hpp"

namespace hx {

Model::Model(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul)
    : thermo_(thermo), hydro_(hydro), foul_(foul) {}

State Model::evaluate(const OperatingPoint &op, double t_seconds) const {
  const double Rf_total = foul_.Rf(t_seconds);
  const double split = foul_.params().split_ratio;
  const double k_deposit = foul_.params().k_deposit;
  
  const double Rf_shell = Rf_total * split;
  const double Rf_tube = Rf_total * (1.0 - split);
  
  State s = thermo_.steady(op, Rf_shell, Rf_tube, k_deposit);
  
  // Tube side is HOT fluid, Shell side is COLD fluid
  s.dP_tube = hydro_.dP_tube(op.m_dot_hot, Rf_tube, k_deposit, thermo_.geometry().K_minor_tube);
  s.dP_shell = hydro_.dP_shell(op.m_dot_cold, Rf_shell, k_deposit, thermo_.geometry().K_turns_shell);
  return s;
}

} // namespace hx
