#include "Fouling.hpp"

#include <algorithm>
#include <cmath>

namespace hx {

Fouling::Fouling(const FoulingParams &p) : p_(p) {}

double Fouling::Rf(double t_seconds) const {
  const double t = std::max(0.0, t_seconds);
  double rf = p_.Rf0;
  if (p_.model == FoulingParams::Model::Asymptotic) {
    rf += p_.RfMax * (1.0 - std::exp(-t / std::max(1e-9, p_.tau)));
  } else {
    rf += p_.alpha * t;
  }
  return std::max(0.0, rf);
}

double Fouling::thickness(double Rf) const {
  // Map Rf to deposit thickness via an effective deposit conductivity (crudely)
  const double k_dep = 0.5; // W/m/K (placeholder)
  return std::max(0.0, Rf) * k_dep;
}

} // namespace hx
