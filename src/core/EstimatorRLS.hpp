#pragma once

namespace hx {

/** \brief Recursive Least Squares estimator for a scalar parameter (e.g., UA or fouling). */
class EstimatorRLS {
public:
  EstimatorRLS(double theta0, double P0, double lambda);
  double update(double y, double phi); // returns theta_hat
  [[nodiscard]] double theta() const { return theta_; }
  [[nodiscard]] double variance() const { return P_; }

private:
  double theta_;
  double P_;
  double lambda_;
};

} // namespace hx
