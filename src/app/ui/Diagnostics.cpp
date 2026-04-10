#include "Diagnostics.hpp"
#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QStandardPaths>
#include <QThread>
#include <iostream>

Diagnostics::Diagnostics() {
  startTime_ = std::chrono::high_resolution_clock::now();
  
  // Capture system info at startup
  qtVersion_ = QLibraryInfo::version().toString();
  osVersion_ = QSysInfo::productType() + " " + QSysInfo::productVersion();
  cpuCoreCount_ = QThread::idealThreadCount();
  
  log(QString("=").repeated(70), Severity::Info);
  log("HeatXTwin Pro - Startup Diagnostics", Severity::Info);
  log(QString("=").repeated(70), Severity::Info);
  log("", Severity::Info);
}

void Diagnostics::beginPhase(const QString& phaseName) {
  activePhases_[phaseName] = std::chrono::high_resolution_clock::now();
  log("→ " + phaseName, Severity::Info);
}

void Diagnostics::endPhase(const QString& details, Severity severity) {
  // Find if any phase is active
  if (activePhases_.empty()) {
    log("ERROR: No active phase to end", Severity::Error);
    return;
  }
  
  // Get the most recent phase
  auto it = activePhases_.rbegin();
  QString phaseName = it->first;
  auto startTime = it->second;
  activePhases_.erase(phaseName.toLatin1().data());
  
  auto endTime = std::chrono::high_resolution_clock::now();
  double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
  
  TimingEntry entry{phaseName, durationMs, details, severity};
  timings_.push_back(entry);
  
  QString statusIcon = (severity == Severity::Error) ? "✗" : 
                       (severity == Severity::Warning) ? "⚠" : "✓";
  
  QString msg = QString("  %1 %2 (%3ms)")
    .arg(statusIcon)
    .arg(phaseName)
    .arg(QString::number(durationMs, 'f', 1));
  
  if (!details.isEmpty()) {
    msg += " - " + details;
  }
  
  log(msg, severity);
}

void Diagnostics::recordTiming(const QString& phaseName, double durationMs,
                               const QString& details, Severity severity) {
  TimingEntry entry{phaseName, durationMs, details, severity};
  timings_.push_back(entry);
  
  QString msg = QString("%1: %2ms")
    .arg(phaseName)
    .arg(QString::number(durationMs, 'f', 1));
  
  if (!details.isEmpty()) {
    msg += " - " + details;
  }
  
  log(msg, severity);
}

void Diagnostics::checkQtVersion() {
  beginPhase("Qt Version Check");
  
  QString qtVer = QLibraryInfo::version().toString();
  QString minimumRequired = "6.0.0";
  
  log("Qt Version: " + qtVer, Severity::Info);
  log("Minimum Required: " + minimumRequired, Severity::Info);
  
  bool passed = QLibraryInfo::version() >= QVersionNumber::fromString(minimumRequired);
  
  CheckResult result{
    "Qt Version",
    passed,
    QString("Qt %1 detected").arg(qtVer),
    passed ? "" : "Upgrade Qt to 6.0.0 or later"
  };
  
  checks_.push_back(result);
  endPhase(qtVer, passed ? Severity::Success : Severity::Warning);
}

void Diagnostics::checkQtPlugins() {
  beginPhase("Qt Plugins Check");
  
  QStringList requiredPlugins = {"qwindows", "qjpeg", "qpng"};
  QStringList missingPlugins;
  
  for (const auto& plugin : requiredPlugins) {
    if (!checkPluginExists("imageformats", plugin) && 
        !checkPluginExists("platforms", plugin)) {
      missingPlugins << plugin;
    }
  }
  
  if (!missingPlugins.isEmpty()) {
    log("Missing plugins: " + missingPlugins.join(", "), Severity::Warning);
    endPhase("Missing " + QString::number(missingPlugins.size()) + " plugins", Severity::Warning);
    warningCount_++;
  } else {
    log("All required plugins found", Severity::Success);
    endPhase("All plugins available", Severity::Success);
  }
  
  CheckResult result{
    "Qt Plugins",
    missingPlugins.isEmpty(),
    QString("Found %1 required plugins").arg(requiredPlugins.size() - missingPlugins.size()),
    missingPlugins.isEmpty() ? "" : "Ensure Qt plugins are in: platforms/, imageformats/ directories"
  };
  
  checks_.push_back(result);
}

void Diagnostics::checkDependencies() {
  beginPhase("Dependencies Check");
  
  // Check for critical DLLs
  QStringList criticalDlls = {
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Charts.dll"
  };
  
  QStringList missingDlls;
  for (const auto& dll : criticalDlls) {
    if (!checkDllExists(dll)) {
      missingDlls << dll;
    }
  }
  
  if (!missingDlls.isEmpty()) {
    log("Missing DLLs: " + missingDlls.join(", "), Severity::Error);
    endPhase("Missing " + QString::number(missingDlls.size()) + " DLLs", Severity::Error);
    errorCount_++;
  } else {
    log("All critical DLLs found", Severity::Success);
    endPhase("All dependencies available", Severity::Success);
  }
  
  CheckResult result{
    "Critical DLLs",
    missingDlls.isEmpty(),
    QString("Found %1/%2 required DLLs").arg(criticalDlls.size() - missingDlls.size()).arg(criticalDlls.size()),
    missingDlls.isEmpty() ? "" : "Copy missing DLLs from Qt installation to application directory"
  };
  
  checks_.push_back(result);
}

void Diagnostics::checkSystemInfo() {
  beginPhase("System Information");
  
  log("OS: " + osVersion_, Severity::Info);
  log("CPU Cores: " + QString::number(cpuCoreCount_), Severity::Info);
  log("Qt Version: " + qtVersion_, Severity::Info);
  log("Architecture: " + QSysInfo::buildAbi(), Severity::Info);
  
  endPhase(osVersion_, Severity::Info);
}

void Diagnostics::runAllChecks() {
  log("", Severity::Info);
  log("Running diagnostic checks...", Severity::Info);
  log("", Severity::Info);
  
  checkSystemInfo();
  log("", Severity::Info);
  checkQtVersion();
  log("", Severity::Info);
  checkQtPlugins();
  log("", Severity::Info);
  checkDependencies();
  log("", Severity::Info);
}

void Diagnostics::log(const QString& message, Severity severity) {
  QString prefix;
  
  switch (severity) {
    case Severity::Info:
      prefix = "[INFO]  ";
      break;
    case Severity::Warning:
      prefix = "[WARN]  ";
      warningCount_++;
      break;
    case Severity::Error:
      prefix = "[ERROR] ";
      errorCount_++;
      break;
    case Severity::Success:
      prefix = "[OK]    ";
      break;
  }
  
  QString logEntry = prefix + message;
  
  // Output to console
  qDebug() << logEntry;
  std::cout << logEntry.toStdString() << std::endl;
}

void Diagnostics::logPhase(const QString& phase, const QString& message, Severity severity) {
  log(phase + ": " + message, severity);
}

void Diagnostics::logCheck(const CheckResult& result) {
  QString status = result.passed ? "[✓]" : "[✗]";
  log(status + " " + result.item + ": " + result.message, 
      result.passed ? Severity::Success : Severity::Error);
  
  if (!result.recommendation.isEmpty()) {
    log("    Recommendation: " + result.recommendation, Severity::Info);
  }
}

QString Diagnostics::getReport() const {
  QString report;
  QTextStream stream(&report);
  
  stream << "HeatXTwin Pro - Startup Diagnostics Report\n";
  stream << "=" << QString("=").repeated(68) << "\n\n";
  
  // System info
  stream << "SYSTEM INFORMATION\n";
  stream << "  OS: " << osVersion_ << "\n";
  stream << "  Qt: " << qtVersion_ << "\n";
  stream << "  CPU Cores: " << cpuCoreCount_ << "\n\n";
  
  // Timings
  if (!timings_.empty()) {
    stream << "STARTUP TIMINGS\n";
    for (const auto& entry : timings_) {
      stream << "  " << entry.phase << ": " << formatDuration(entry.durationMs);
      if (!entry.details.isEmpty()) {
        stream << " (" << entry.details << ")";
      }
      stream << "\n";
    }
    stream << "  TOTAL: " << formatDuration(getTotalStartupTime()) << "\n\n";
  }
  
  // Checks
  if (!checks_.empty()) {
    stream << "DEPENDENCY CHECKS\n";
    for (const auto& check : checks_) {
      QString status = check.passed ? "[✓]" : "[✗]";
      stream << "  " << status << " " << check.item << ": " << check.message << "\n";
      if (!check.recommendation.isEmpty()) {
        stream << "      Recommendation: " << check.recommendation << "\n";
      }
    }
    stream << "\n";
  }
  
  // Summary
  stream << "SUMMARY\n";
  stream << "  Errors: " << errorCount_ << "\n";
  stream << "  Warnings: " << warningCount_ << "\n";
  stream << "  Status: " << (hasErrors() ? "FAILED" : "PASSED") << "\n";
  
  return report;
}

void Diagnostics::exportToLog(const QString& filepath) const {
  QFile file(filepath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return;
  }
  
  QTextStream out(&file);
  out << getReport();
  file.close();
}

void Diagnostics::exportToJson(const QString& filepath) const {
  QJsonDocument doc(toJsonObject());
  
  QFile file(filepath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return;
  }
  
  file.write(doc.toJson());
  file.close();
}

void Diagnostics::printSummary() const {
  std::cout << "\n" << getReport().toStdString() << std::endl;
}

double Diagnostics::getTotalStartupTime() const {
  if (timings_.empty()) return 0.0;
  
  double total = 0.0;
  for (const auto& entry : timings_) {
    total += entry.durationMs;
  }
  return total;
}

QString Diagnostics::formatDuration(double ms) {
  if (ms < 1000) {
    return QString::number(ms, 'f', 1) + " ms";
  }
  return QString::number(ms / 1000.0, 'f', 2) + " s";
}

QString Diagnostics::severityToString(Severity sev) {
  switch (sev) {
    case Severity::Info: return "INFO";
    case Severity::Warning: return "WARNING";
    case Severity::Error: return "ERROR";
    case Severity::Success: return "SUCCESS";
  }
  return "UNKNOWN";
}

QString Diagnostics::severityColor(Severity sev) {
  switch (sev) {
    case Severity::Info: return "\033[36m";      // Cyan
    case Severity::Warning: return "\033[33m";   // Yellow
    case Severity::Error: return "\033[31m";     // Red
    case Severity::Success: return "\033[32m";   // Green
  }
  return "";
}

// === PRIVATE HELPERS ===

QString Diagnostics::getQtPluginPath() const {
  return QStandardPaths::locate(QStandardPaths::ApplicationsLocation, 
                               QDir::currentPath(), 
                               QStandardPaths::LocateDirectory);
}

bool Diagnostics::checkDllExists(const QString& dllName) const {
  // Check in current directory
  if (QFile::exists(dllName)) return true;
  
  // Check in system paths
  QStringList searchPaths = {
    QApplication::applicationDirPath(),
    QApplication::applicationDirPath() + "/../lib"
  };
  
  for (const auto& path : searchPaths) {
    if (QFile::exists(path + "/" + dllName)) return true;
  }
  
  return false;
}

bool Diagnostics::checkPluginExists(const QString& pluginType, const QString& pluginName) const {
  QString pluginPath = QApplication::applicationDirPath() + "/" + pluginType;
  
  QDir pluginDir(pluginPath);
  if (!pluginDir.exists()) return false;
  
  // Check for .dll or .so files
  for (const auto& file : pluginDir.entryList(QDir::Files)) {
    if (file.contains(pluginName, Qt::CaseInsensitive)) {
      return true;
    }
  }
  
  return false;
}

QJsonObject Diagnostics::toJsonObject() const {
  QJsonObject root;
  
  // System info
  QJsonObject sysInfo;
  sysInfo["os"] = osVersion_;
  sysInfo["qt_version"] = qtVersion_;
  sysInfo["cpu_cores"] = cpuCoreCount_;
  sysInfo["arch"] = QSysInfo::buildAbi();
  root["system"] = sysInfo;
  
  // Timings
  QJsonArray timingsArray;
  for (const auto& entry : timings_) {
    QJsonObject timing;
    timing["phase"] = entry.phase;
    timing["duration_ms"] = entry.durationMs;
    timing["details"] = entry.details;
    timing["severity"] = severityToString(entry.severity);
    timingsArray.append(timing);
  }
  root["timings"] = timingsArray;
  
  // Checks
  QJsonArray checksArray;
  for (const auto& check : checks_) {
    QJsonObject checkObj;
    checkObj["item"] = check.item;
    checkObj["passed"] = check.passed;
    checkObj["message"] = check.message;
    checkObj["recommendation"] = check.recommendation;
    checksArray.append(checkObj);
  }
  root["checks"] = checksArray;
  
  // Summary
  QJsonObject summary;
  summary["total_startup_time_ms"] = getTotalStartupTime();
  summary["error_count"] = errorCount_;
  summary["warning_count"] = warningCount_;
  summary["status"] = hasErrors() ? "FAILED" : "PASSED";
  root["summary"] = summary;
  
  return root;
}
