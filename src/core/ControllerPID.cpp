#include "ControllerPID.hpp"

#include <algorithm>

namespace hx {

ControllerPID::ControllerPID(double kp, double ki, double kd, double umin, double umax, double rateLimit)
    : kp_(kp), ki_(ki), kd_(kd), umin_(umin), umax_(umax), rate_(rateLimit) {}

double ControllerPID::update(double sp, double y, double dt) {
  auto clampd = [](double x, double lo, double hi) {
    return x < lo ? lo : (x > hi ? hi : x);
  };
  const double err = sp - y;
  ui_ += ki_ * err * dt;
  // anti-windup clamp
  ui_ = clampd(ui_, umin_, umax_);
  const double ud = kd_ * (err - prevErr_) / std::max(1e-9, dt);
  const double u_new = kp_ * err + ui_ + ud;
  // rate limit
  const double du = clampd(u_new - u_, -rate_ * dt, rate_ * dt);
  u_ = clampd(u_ + du, umin_, umax_);
  prevErr_ = err;
  return u_;
}

} // namespace hx
