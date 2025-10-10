#pragma once

#include <fstream>

#include "core/Types.hpp"

namespace io {

class CsvLogger {
public:
  bool open(const std::string &path);
  void write(double t, const hx::State &s);
  void close();
  bool isOpen() const { return ofs_.is_open(); }

private:
  std::ofstream ofs_;
};

} // namespace io
