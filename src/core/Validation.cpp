#include "Validation.hpp"

#include <cmath>

namespace hx {

bool Validation::is_finite(double x) { return std::isfinite(x); }

} // namespace hx
