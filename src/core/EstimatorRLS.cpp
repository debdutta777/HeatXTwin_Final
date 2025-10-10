#include "EstimatorRLS.hpp"

namespace hx {

EstimatorRLS::EstimatorRLS(double theta0, double P0, double lambda) : theta_(theta0), P_(P0), lambda_(lambda) {}

double EstimatorRLS::update(double y, double phi) {
  // K = P*phi / (lambda + phi^2 * P)
  const double denom = lambda_ + phi * phi * P_;
  const double K = (P_ * phi) / denom;
  // theta = theta + K*(y - phi*theta)
  theta_ = theta_ + K * (y - phi * theta_);
  // P = (1/lambda)*(P - K*phi*P)
  P_ = (1.0 / lambda_) * (P_ - K * phi * P_);
  return theta_;
}

} // namespace hx
