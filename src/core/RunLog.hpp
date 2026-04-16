#pragma once

#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <vector>

#include "core/Types.hpp"
#include "core/Simulator.hpp"

namespace hx {

/**
 *  One row of a compact run-history record.  Everything needed to reconstruct
 *  the exchanger's identity + headline KPIs — no time-series data.  Full
 *  sample traces live in the \c samples table and are fetched on demand.
 */
struct RunRecord {
  qint64   id              = -1;      // SQLite primary key (unique, monotonic)
  QString  started_at_iso;            // ISO-8601 local time
  double   duration_s      = 0.0;     // wall-clock run length
  QString  mode;                      // e.g. "Dynamic with Fouling"
  QString  notes;                     // optional user label

  // Geometry snapshot
  int      n_tubes         = 0;
  double   Do = 0.0, Di = 0.0, L = 0.0, pitch = 0.0, shell_ID = 0.0;
  double   baffle_spacing  = 0.0, baffle_cut = 0.0;
  int      n_baffles       = 0;
  QString  shell_method;              // "Kern" or "Bell-Delaware"
  QString  arrangement;

  // Fluids (4 intensive props each)
  double   hot_rho = 0.0, hot_mu = 0.0, hot_cp = 0.0, hot_k = 0.0;
  double   cold_rho = 0.0, cold_mu = 0.0, cold_cp = 0.0, cold_k = 0.0;

  // Operating point
  double   m_dot_hot  = 0.0, m_dot_cold = 0.0;
  double   Tin_hot    = 0.0, Tin_cold   = 0.0;

  // Final-state KPIs (after the last sample)
  double   Tc_out_final  = 0.0, Th_out_final = 0.0;
  double   Q_final       = 0.0, U_final = 0.0, Rf_final = 0.0;
  double   dP_tube_final = 0.0, dP_shell_final = 0.0;

  // PID control
  bool     pid_enabled = false;
  double   pid_sp = 0.0, pid_kp = 0.0, pid_ki = 0.0, pid_kd = 0.0;
};

/** One stored time-series sample.  Mirrors \c hx::State but in DB-friendly POD form. */
struct RunSample {
  double t;
  double Tc_out, Th_out;
  double Q, U, Rf;
  double dP_tube, dP_shell;
  double pid_cmd;          // NaN if PID disabled for this run
};

/**
 *  \brief SQLite-backed run history.
 *
 *  A process-wide singleton that opens \c runs.db under
 *  \c QStandardPaths::AppLocalDataLocation on first use, creates the schema
 *  if missing, and exposes CRUD helpers used by both the simulator (auto-log
 *  on simulation finish) and the \c RunLogDialog (browse / compare / delete).
 *
 *  All methods are safe to call from the GUI thread only.
 */
class RunLog {
public:
  static RunLog &instance();

  /** Absolute path of the backing SQLite file (for display/copy/export). */
  QString databasePath() const { return dbPath_; }

  /** Returns false if the DB could not be opened (permissions, corrupt, …). */
  bool isOpen() const { return db_.isOpen(); }

  /** Persist one run + all its samples.  Returns the new row id (or -1). */
  qint64 saveRun(const RunRecord               &rec,
                  const std::vector<RunSample> &samples);

  /** Fetch all runs, most recent first. */
  std::vector<RunRecord> listRuns();

  /** Fetch time-series samples for one run in chronological order. */
  std::vector<RunSample> loadSamples(qint64 runId);

  /** Remove a run and its samples (FK cascade).  Returns true on success. */
  bool deleteRun(qint64 runId);

  /** Attach a free-form note to a stored run. */
  bool updateNotes(qint64 runId, const QString &notes);

  /**
   *  Helper that builds a \c RunRecord from the structs the simulator
   *  already owns.  The samples vector is not consumed here — pass it to
   *  \c saveRun() after recording ends.
   */
  static RunRecord makeRecord(const SimConfig      &cfg,
                               const OperatingPoint &op,
                               const Geometry       &g,
                               const Fluid          &hot,
                               const Fluid          &cold,
                               const QString        &modeLabel,
                               double                duration_s,
                               const State          &finalState);

private:
  RunLog();
  RunLog(const RunLog&)            = delete;
  RunLog &operator=(const RunLog&) = delete;

  bool ensureSchema();

  QSqlDatabase db_;
  QString      dbPath_;
  bool         schemaReady_{false};
};

} // namespace hx
