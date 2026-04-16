#pragma once

#include <QDialog>
#include "core/RunLog.hpp"

class QTableWidget;
class QTableWidgetItem;
class QPushButton;
class QLabel;
class QPlainTextEdit;

class QChart;
class QChartView;

/**
 *  \brief Browse & compare historical runs stored in the SQLite run log.
 *
 *  Two-pane layout:
 *    - Top:    sortable table of all runs (id, time, mode, Tc_out, Q, U, Rf, ΔP).
 *    - Bottom: comparison chart that overlays Tc_out(t) and Q(t) for up to
 *              two selected rows, with a delta-summary label.
 *
 *  Users can rename notes, delete runs, and copy the DB path for archival.
 */
class RunLogDialog : public QDialog {
  Q_OBJECT
public:
  explicit RunLogDialog(QWidget *parent = nullptr);

private slots:
  void refresh();
  void onSelectionChanged();
  void onDeleteSelected();
  void onRenameNote();
  void onExportCsv();

private:
  void populateTable(const std::vector<hx::RunRecord> &rows);
  void updateComparison();

  QTableWidget *tbl_{};
  QChartView   *chartTemp_{};
  QChartView   *chartHeat_{};
  QLabel       *lblSummary_{};
  QPushButton  *btnDelete_{};
  QPushButton  *btnRename_{};
  QPushButton  *btnExport_{};
  QPushButton  *btnRefresh_{};

  std::vector<hx::RunRecord> rows_;
};
