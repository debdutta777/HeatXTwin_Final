#include "CsvLogger.hpp"

#include <iomanip>

namespace io {

bool CsvLogger::open(const std::string &path) {
  ofs_.open(path, std::ios::out | std::ios::trunc);
  if (!ofs_) return false;
  ofs_ << "t,Tc_out,Th_out,Q,U,Rf,dP_tube,dP_shell\n";
  return true;
}

void CsvLogger::write(double t, const hx::State &s) {
  if (!ofs_) return;
  ofs_ << std::fixed << std::setprecision(6)
       << t << ',' << s.Tc_out << ',' << s.Th_out << ',' << s.Q << ',' << s.U << ','
       << s.Rf << ',' << s.dP_tube << ',' << s.dP_shell << "\n";
}

void CsvLogger::close() {
  if (ofs_) ofs_.close();
}

} // namespace io
