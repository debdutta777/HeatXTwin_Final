#pragma once

namespace hx {

/** \brief Simple PID controller with clamp and rate limit. */
class ControllerPID {
public:
  ControllerPID(double kp, double ki, double kd, double umin, double umax, double rateLimit);
  double update(double sp, double y, double dt);

private:
  double kp_, ki_, kd_;
  double umin_, umax_, rate_;
  double u_{0.0}, ui_{0.0}, prevErr_{0.0};
};

} // namespace hx
