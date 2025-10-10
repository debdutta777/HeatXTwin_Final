#include "Model.hpp"

namespace hx {

Model::Model(const Thermo &thermo, const Hydraulics &hydro, const Fouling &foul)
    : thermo_(thermo), hydro_(hydro), foul_(foul) {}

State Model::evaluate(const OperatingPoint &op, double t_seconds) const {
  const double Rf_shell = foul_.Rf(t_seconds) * 0.5; // split Rf arbitrarily between shell and tube for now
  const double Rf_tube = foul_.Rf(t_seconds) * 0.5;
  State s = thermo_.steady(op, Rf_shell, Rf_tube);
  s.dP_tube = hydro_.dP_tube(op.m_dot_cold, Rf_tube);
  s.dP_shell = hydro_.dP_shell(op.m_dot_hot, Rf_shell);
  return s;
}

} // namespace hx
