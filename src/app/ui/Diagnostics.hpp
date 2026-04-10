#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <chrono>
#include <memory>
#include <map>
#include <vector>

/**
 * @class Diagnostics
 * @brief Comprehensive startup diagnostics and performance monitoring
 * 
 * Tracks:
 * - Application initialization timing
 * - Qt version and platform information
 * - Dependency availability (DLLs, plugins)
 * - Performance metrics during startup
 * - System information (OS, CPU, Memory)
 * 
 * Outputs to:
 * - Console (real-time)
 * - Startup.log (persistent)
 * - Startup.json (structured data for parsing)
 */
class Diagnostics {
public:
  enum class Severity {
    Info,
    Warning,
    Error,
    Success
  };

  struct TimingEntry {
    QString phase;
    double durationMs;
    QString details;
    Severity severity;
  };

  struct CheckResult {
    QString item;
    bool passed;
    QString message;
    QString recommendation;
  };

  // Constructor
  Diagnostics();

  // === TIMING & PHASES ===
  
  /// Start timing for a phase
  void beginPhase(const QString& phaseName);
  
  /// End timing for current phase
  void endPhase(const QString& details = "", Severity severity = Severity::Info);
  
  /// Record a single timing measurement
  void recordTiming(const QString& phaseName, double durationMs, 
                   const QString& details = "", Severity severity = Severity::Info);

  // === ENVIRONMENT CHECKS ===
  
  /// Check Qt version compatibility
  void checkQtVersion();
  
  /// Check available Qt plugins
  void checkQtPlugins();
  
  /// Check required DLLs
  void checkDependencies();
  
  /// Check system information
  void checkSystemInfo();
  
  /// Run all checks (recommended for startup)
  void runAllChecks();

  // === LOGGING ===
  
  /// Log a message with severity level
  void log(const QString& message, Severity severity = Severity::Info);
  
  /// Log in formatted style (phase: message)
  void logPhase(const QString& phase, const QString& message, Severity severity = Severity::Info);
  
  /// Log a check result
  void logCheck(const CheckResult& result);

  // === REPORTING ===
  
  /// Get formatted startup report
  QString getReport() const;
  
  /// Export diagnostics to JSON file
  void exportToJson(const QString& filepath) const;
  
  /// Export diagnostics to log file
  void exportToLog(const QString& filepath) const;
  
  /// Print summary to console
  void printSummary() const;

  // === GETTERS ===
  
  /// Get total startup time in milliseconds
  double getTotalStartupTime() const;
  
  /// Get all timing entries
  const std::vector<TimingEntry>& getTimings() const { return timings_; }
  
  /// Get all check results
  const std::vector<CheckResult>& getChecks() const { return checks_; }
  
  /// Check if any errors occurred
  bool hasErrors() const { return errorCount_ > 0; }
  
  /// Get error count
  int getErrorCount() const { return errorCount_; }
  
  /// Get warning count
  int getWarningCount() const { return warningCount_; }

  // === UTILITY ===
  
  /// Format duration in human-readable format
  static QString formatDuration(double ms);
  
  /// Get severity as string
  static QString severityToString(Severity sev);
  
  /// Get severity color code for console output
  static QString severityColor(Severity sev);

private:
  // Timing data
  std::vector<TimingEntry> timings_;
  std::map<QString, std::chrono::high_resolution_clock::time_point> activePhases_;
  std::chrono::high_resolution_clock::time_point startTime_;
  
  // Check results
  std::vector<CheckResult> checks_;
  
  // Statistics
  int errorCount_ = 0;
  int warningCount_ = 0;
  
  // System info
  QString osVersion_;
  QString qtVersion_;
  int cpuCoreCount_ = 0;
  long long totalMemory_ = 0;
  
  // Internal helpers
  QString getQtPluginPath() const;
  bool checkDllExists(const QString& dllName) const;
  bool checkPluginExists(const QString& pluginType, const QString& pluginName) const;
  QJsonObject toJsonObject() const;
};

// Convenience macros for timing
#define DIAG_BEGIN(diag, phase) (diag).beginPhase(phase)
#define DIAG_END(diag, details) (diag).endPhase(details)
#define DIAG_LOG(diag, msg) (diag).log(msg, Diagnostics::Severity::Info)
#define DIAG_WARN(diag, msg) (diag).log(msg, Diagnostics::Severity::Warning)
#define DIAG_ERROR(diag, msg) (diag).log(msg, Diagnostics::Severity::Error)
