#include "RunLog.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

namespace hx {

namespace {

constexpr const char *kConnName = "hx_runlog";

QString shellMethodLabel(ShellSideMethod m) {
  return m == ShellSideMethod::BellDelaware ? QStringLiteral("Bell-Delaware")
                                             : QStringLiteral("Kern");
}

QString arrangementLabel(FlowArrangement a) {
  switch (a) {
    case FlowArrangement::CounterFlow:    return QStringLiteral("Counter-flow");
    case FlowArrangement::ParallelFlow:   return QStringLiteral("Parallel-flow");
    case FlowArrangement::ShellTube_1_2:  return QStringLiteral("1-2 Shell-&-Tube");
    case FlowArrangement::ShellTube_2_4:  return QStringLiteral("2-4 Shell-&-Tube");
  }
  return QStringLiteral("Counter-flow");
}

} // anonymous namespace

RunLog &RunLog::instance() {
  static RunLog s;
  return s;
}

RunLog::RunLog() {
  QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  if (dir.isEmpty())
    dir = QCoreApplication::applicationDirPath();
  QDir().mkpath(dir);
  dbPath_ = QDir(dir).absoluteFilePath(QStringLiteral("runs.db"));

  if (QSqlDatabase::contains(kConnName)) {
    db_ = QSqlDatabase::database(kConnName);
  } else {
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnName);
    db_.setDatabaseName(dbPath_);
  }

  if (!db_.open()) {
    qWarning() << "[RunLog] cannot open" << dbPath_ << ":" << db_.lastError().text();
    return;
  }

  // Enable foreign keys + ON DELETE CASCADE
  QSqlQuery pragma(db_);
  pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
  pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));

  schemaReady_ = ensureSchema();
}

bool RunLog::ensureSchema() {
  QSqlQuery q(db_);

  const char *runsDDL = R"SQL(
    CREATE TABLE IF NOT EXISTS runs (
      id               INTEGER PRIMARY KEY AUTOINCREMENT,
      started_at       TEXT    NOT NULL,
      duration_s       REAL    NOT NULL DEFAULT 0,
      mode             TEXT,
      notes            TEXT,
      n_tubes          INTEGER, Do REAL, Di REAL, L REAL, pitch REAL, shell_ID REAL,
      baffle_spacing   REAL, baffle_cut REAL, n_baffles INTEGER,
      shell_method     TEXT, arrangement TEXT,
      hot_rho  REAL, hot_mu  REAL, hot_cp  REAL, hot_k  REAL,
      cold_rho REAL, cold_mu REAL, cold_cp REAL, cold_k REAL,
      m_dot_hot REAL, m_dot_cold REAL, Tin_hot REAL, Tin_cold REAL,
      Tc_out_final REAL, Th_out_final REAL,
      Q_final REAL, U_final REAL, Rf_final REAL,
      dP_tube_final REAL, dP_shell_final REAL,
      pid_enabled INTEGER, pid_sp REAL, pid_kp REAL, pid_ki REAL, pid_kd REAL
    );)SQL";

  if (!q.exec(QString::fromUtf8(runsDDL))) {
    qWarning() << "[RunLog] runs DDL failed:" << q.lastError().text();
    return false;
  }

  const char *samplesDDL = R"SQL(
    CREATE TABLE IF NOT EXISTS samples (
      run_id   INTEGER NOT NULL,
      t        REAL    NOT NULL,
      Tc_out   REAL, Th_out REAL,
      Q        REAL, U REAL, Rf REAL,
      dP_tube  REAL, dP_shell REAL,
      pid_cmd  REAL,
      FOREIGN KEY(run_id) REFERENCES runs(id) ON DELETE CASCADE
    );)SQL";
  if (!q.exec(QString::fromUtf8(samplesDDL))) {
    qWarning() << "[RunLog] samples DDL failed:" << q.lastError().text();
    return false;
  }
  q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_samples_run ON samples(run_id);"));

  return true;
}

RunRecord RunLog::makeRecord(const SimConfig      &cfg,
                              const OperatingPoint &op,
                              const Geometry       &g,
                              const Fluid          &hot,
                              const Fluid          &cold,
                              const QString        &modeLabel,
                              double                duration_s,
                              const State          &finalState) {
  RunRecord r;
  r.started_at_iso = QDateTime::currentDateTime().toString(Qt::ISODate);
  r.duration_s = duration_s;
  r.mode       = modeLabel;

  r.n_tubes  = g.nTubes;
  r.Do = g.Do; r.Di = g.Di; r.L = g.L;
  r.pitch = g.pitch;  r.shell_ID = g.shellID;
  r.baffle_spacing = g.baffleSpacing;
  r.baffle_cut = g.baffleCutFrac;
  r.n_baffles  = g.nBaffles;
  r.shell_method = shellMethodLabel(cfg.shellMethod);
  r.arrangement  = arrangementLabel(cfg.arrangement);

  r.hot_rho = hot.rho; r.hot_mu = hot.mu; r.hot_cp = hot.cp; r.hot_k = hot.k;
  r.cold_rho = cold.rho; r.cold_mu = cold.mu; r.cold_cp = cold.cp; r.cold_k = cold.k;

  r.m_dot_hot = op.m_dot_hot; r.m_dot_cold = op.m_dot_cold;
  r.Tin_hot   = op.Tin_hot;   r.Tin_cold   = op.Tin_cold;

  r.Tc_out_final  = finalState.Tc_out;
  r.Th_out_final  = finalState.Th_out;
  r.Q_final       = finalState.Q;
  r.U_final       = finalState.U;
  r.Rf_final      = finalState.Rf;
  r.dP_tube_final = finalState.dP_tube;
  r.dP_shell_final = finalState.dP_shell;

  r.pid_enabled = cfg.pid.enabled;
  r.pid_sp = cfg.pid.setpoint_Tc_out;
  r.pid_kp = cfg.pid.kp; r.pid_ki = cfg.pid.ki; r.pid_kd = cfg.pid.kd;

  return r;
}

qint64 RunLog::saveRun(const RunRecord              &rec,
                        const std::vector<RunSample> &samples) {
  if (!schemaReady_) return -1;

  QSqlQuery q(db_);
  db_.transaction();

  q.prepare(QStringLiteral(
      "INSERT INTO runs ("
      " started_at, duration_s, mode, notes,"
      " n_tubes, Do, Di, L, pitch, shell_ID, baffle_spacing, baffle_cut, n_baffles,"
      " shell_method, arrangement,"
      " hot_rho, hot_mu, hot_cp, hot_k,"
      " cold_rho, cold_mu, cold_cp, cold_k,"
      " m_dot_hot, m_dot_cold, Tin_hot, Tin_cold,"
      " Tc_out_final, Th_out_final, Q_final, U_final, Rf_final,"
      " dP_tube_final, dP_shell_final,"
      " pid_enabled, pid_sp, pid_kp, pid_ki, pid_kd) VALUES ("
      " ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
      " ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
      " ?, ?, ?, ?,"
      " ?, ?, ?, ?, ?,"
      " ?, ?,"
      " ?, ?, ?, ?, ?)"));

  q.addBindValue(rec.started_at_iso);
  q.addBindValue(rec.duration_s);
  q.addBindValue(rec.mode);
  q.addBindValue(rec.notes);
  q.addBindValue(rec.n_tubes);
  q.addBindValue(rec.Do); q.addBindValue(rec.Di); q.addBindValue(rec.L);
  q.addBindValue(rec.pitch); q.addBindValue(rec.shell_ID);
  q.addBindValue(rec.baffle_spacing); q.addBindValue(rec.baffle_cut);
  q.addBindValue(rec.n_baffles);
  q.addBindValue(rec.shell_method); q.addBindValue(rec.arrangement);
  q.addBindValue(rec.hot_rho);  q.addBindValue(rec.hot_mu);
  q.addBindValue(rec.hot_cp);   q.addBindValue(rec.hot_k);
  q.addBindValue(rec.cold_rho); q.addBindValue(rec.cold_mu);
  q.addBindValue(rec.cold_cp);  q.addBindValue(rec.cold_k);
  q.addBindValue(rec.m_dot_hot); q.addBindValue(rec.m_dot_cold);
  q.addBindValue(rec.Tin_hot);   q.addBindValue(rec.Tin_cold);
  q.addBindValue(rec.Tc_out_final); q.addBindValue(rec.Th_out_final);
  q.addBindValue(rec.Q_final); q.addBindValue(rec.U_final); q.addBindValue(rec.Rf_final);
  q.addBindValue(rec.dP_tube_final); q.addBindValue(rec.dP_shell_final);
  q.addBindValue(rec.pid_enabled ? 1 : 0);
  q.addBindValue(rec.pid_sp); q.addBindValue(rec.pid_kp);
  q.addBindValue(rec.pid_ki); q.addBindValue(rec.pid_kd);

  if (!q.exec()) {
    qWarning() << "[RunLog] insert run failed:" << q.lastError().text();
    db_.rollback();
    return -1;
  }

  const qint64 runId = q.lastInsertId().toLongLong();

  // Samples batch
  q.prepare(QStringLiteral(
      "INSERT INTO samples (run_id, t, Tc_out, Th_out, Q, U, Rf, dP_tube, dP_shell, pid_cmd) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
  for (const auto &s : samples) {
    q.bindValue(0, runId);
    q.bindValue(1, s.t);
    q.bindValue(2, s.Tc_out);
    q.bindValue(3, s.Th_out);
    q.bindValue(4, s.Q);
    q.bindValue(5, s.U);
    q.bindValue(6, s.Rf);
    q.bindValue(7, s.dP_tube);
    q.bindValue(8, s.dP_shell);
    q.bindValue(9, s.pid_cmd);
    if (!q.exec()) {
      qWarning() << "[RunLog] sample insert failed:" << q.lastError().text();
      db_.rollback();
      return -1;
    }
  }

  if (!db_.commit()) {
    qWarning() << "[RunLog] commit failed:" << db_.lastError().text();
    return -1;
  }
  return runId;
}

std::vector<RunRecord> RunLog::listRuns() {
  std::vector<RunRecord> out;
  if (!schemaReady_) return out;
  QSqlQuery q(db_);
  if (!q.exec(QStringLiteral("SELECT * FROM runs ORDER BY id DESC"))) {
    qWarning() << "[RunLog] list failed:" << q.lastError().text();
    return out;
  }
  auto idx = [&q](const char *name) { return q.record().indexOf(QLatin1String(name)); };
  int iId = -1, iAt = -1, iDur = -1, iMode = -1, iNotes = -1;
  int iNT = -1, iDo = -1, iDi = -1, iL = -1, iPitch = -1, iShell = -1;
  int iBs = -1, iBc = -1, iNB = -1, iSm = -1, iArr = -1;
  int iHrho = -1, iHmu = -1, iHcp = -1, iHk = -1;
  int iCrho = -1, iCmu = -1, iCcp = -1, iCk = -1;
  int iMh = -1, iMc = -1, iTh = -1, iTc = -1;
  int iTcOF = -1, iThOF = -1, iQF = -1, iUF = -1, iRfF = -1;
  int iDPtF = -1, iDPsF = -1;
  int iPidEn = -1, iPidSp = -1, iPidKp = -1, iPidKi = -1, iPidKd = -1;

  if (q.first()) {
    iId = idx("id"); iAt = idx("started_at"); iDur = idx("duration_s");
    iMode = idx("mode"); iNotes = idx("notes");
    iNT = idx("n_tubes"); iDo = idx("Do"); iDi = idx("Di"); iL = idx("L");
    iPitch = idx("pitch"); iShell = idx("shell_ID");
    iBs = idx("baffle_spacing"); iBc = idx("baffle_cut"); iNB = idx("n_baffles");
    iSm = idx("shell_method"); iArr = idx("arrangement");
    iHrho = idx("hot_rho"); iHmu = idx("hot_mu");
    iHcp = idx("hot_cp");   iHk  = idx("hot_k");
    iCrho = idx("cold_rho"); iCmu = idx("cold_mu");
    iCcp = idx("cold_cp");   iCk  = idx("cold_k");
    iMh = idx("m_dot_hot"); iMc = idx("m_dot_cold");
    iTh = idx("Tin_hot");   iTc = idx("Tin_cold");
    iTcOF = idx("Tc_out_final"); iThOF = idx("Th_out_final");
    iQF = idx("Q_final"); iUF = idx("U_final"); iRfF = idx("Rf_final");
    iDPtF = idx("dP_tube_final"); iDPsF = idx("dP_shell_final");
    iPidEn = idx("pid_enabled"); iPidSp = idx("pid_sp");
    iPidKp = idx("pid_kp"); iPidKi = idx("pid_ki"); iPidKd = idx("pid_kd");

    do {
      RunRecord r;
      r.id = q.value(iId).toLongLong();
      r.started_at_iso = q.value(iAt).toString();
      r.duration_s = q.value(iDur).toDouble();
      r.mode = q.value(iMode).toString();
      r.notes = q.value(iNotes).toString();
      r.n_tubes = q.value(iNT).toInt();
      r.Do = q.value(iDo).toDouble(); r.Di = q.value(iDi).toDouble();
      r.L = q.value(iL).toDouble();   r.pitch = q.value(iPitch).toDouble();
      r.shell_ID = q.value(iShell).toDouble();
      r.baffle_spacing = q.value(iBs).toDouble();
      r.baffle_cut = q.value(iBc).toDouble();
      r.n_baffles = q.value(iNB).toInt();
      r.shell_method = q.value(iSm).toString();
      r.arrangement = q.value(iArr).toString();
      r.hot_rho = q.value(iHrho).toDouble(); r.hot_mu = q.value(iHmu).toDouble();
      r.hot_cp  = q.value(iHcp).toDouble();  r.hot_k  = q.value(iHk).toDouble();
      r.cold_rho = q.value(iCrho).toDouble(); r.cold_mu = q.value(iCmu).toDouble();
      r.cold_cp  = q.value(iCcp).toDouble();  r.cold_k  = q.value(iCk).toDouble();
      r.m_dot_hot = q.value(iMh).toDouble(); r.m_dot_cold = q.value(iMc).toDouble();
      r.Tin_hot = q.value(iTh).toDouble();   r.Tin_cold = q.value(iTc).toDouble();
      r.Tc_out_final = q.value(iTcOF).toDouble();
      r.Th_out_final = q.value(iThOF).toDouble();
      r.Q_final = q.value(iQF).toDouble();
      r.U_final = q.value(iUF).toDouble();
      r.Rf_final = q.value(iRfF).toDouble();
      r.dP_tube_final  = q.value(iDPtF).toDouble();
      r.dP_shell_final = q.value(iDPsF).toDouble();
      r.pid_enabled = q.value(iPidEn).toInt() != 0;
      r.pid_sp = q.value(iPidSp).toDouble();
      r.pid_kp = q.value(iPidKp).toDouble();
      r.pid_ki = q.value(iPidKi).toDouble();
      r.pid_kd = q.value(iPidKd).toDouble();
      out.push_back(std::move(r));
    } while (q.next());
  }
  return out;
}

std::vector<RunSample> RunLog::loadSamples(qint64 runId) {
  std::vector<RunSample> out;
  if (!schemaReady_) return out;
  QSqlQuery q(db_);
  q.prepare(QStringLiteral(
      "SELECT t, Tc_out, Th_out, Q, U, Rf, dP_tube, dP_shell, pid_cmd "
      "FROM samples WHERE run_id = ? ORDER BY t ASC"));
  q.addBindValue(runId);
  if (!q.exec()) {
    qWarning() << "[RunLog] loadSamples failed:" << q.lastError().text();
    return out;
  }
  while (q.next()) {
    RunSample s;
    s.t = q.value(0).toDouble();
    s.Tc_out = q.value(1).toDouble();
    s.Th_out = q.value(2).toDouble();
    s.Q = q.value(3).toDouble();
    s.U = q.value(4).toDouble();
    s.Rf = q.value(5).toDouble();
    s.dP_tube = q.value(6).toDouble();
    s.dP_shell = q.value(7).toDouble();
    s.pid_cmd = q.value(8).toDouble();
    out.push_back(s);
  }
  return out;
}

bool RunLog::deleteRun(qint64 runId) {
  if (!schemaReady_) return false;
  QSqlQuery q(db_);
  q.prepare(QStringLiteral("DELETE FROM runs WHERE id = ?"));
  q.addBindValue(runId);
  if (!q.exec()) {
    qWarning() << "[RunLog] delete run failed:" << q.lastError().text();
    return false;
  }
  // Samples cascade via FK
  return q.numRowsAffected() > 0;
}

bool RunLog::updateNotes(qint64 runId, const QString &notes) {
  if (!schemaReady_) return false;
  QSqlQuery q(db_);
  q.prepare(QStringLiteral("UPDATE runs SET notes = ? WHERE id = ?"));
  q.addBindValue(notes);
  q.addBindValue(runId);
  if (!q.exec()) {
    qWarning() << "[RunLog] update notes failed:" << q.lastError().text();
    return false;
  }
  return q.numRowsAffected() > 0;
}

} // namespace hx
