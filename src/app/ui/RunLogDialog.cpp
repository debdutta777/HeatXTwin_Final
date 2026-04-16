#include "RunLogDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QLocale>
#include <QTextStream>
#include <QFile>
#include <QApplication>
#include <QClipboard>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <cmath>

namespace {

QString fmt(double v, int prec = 3) {
  if (!std::isfinite(v)) return QStringLiteral("—");
  return QLocale::c().toString(v, 'g', prec);
}

QChartView *makeChart(const QString &title, const QString &yTitle,
                       const QString &yUnits) {
  auto *chart = new QChart();
  chart->setTitle(title);
  chart->legend()->setAlignment(Qt::AlignBottom);
  chart->setAnimationOptions(QChart::NoAnimation);

  auto *ax = new QValueAxis();  ax->setTitleText(QStringLiteral("t [s]"));
  auto *ay = new QValueAxis();  ay->setTitleText(yTitle + QStringLiteral(" [") + yUnits + QStringLiteral("]"));
  chart->addAxis(ax, Qt::AlignBottom);
  chart->addAxis(ay, Qt::AlignLeft);

  auto *view = new QChartView(chart);
  view->setRenderHint(QPainter::Antialiasing);
  view->setMinimumHeight(220);
  return view;
}

void plotRunOnChart(QChart *chart, const hx::RunRecord &rec,
                     const std::vector<hx::RunSample> &samples,
                     int which /* 0=Tc_out, 1=Q */,
                     QColor color) {
  if (samples.empty()) return;
  auto *series = new QLineSeries();
  series->setName(QStringLiteral("#%1  %2").arg(rec.id).arg(rec.mode));
  QPen pen(color);  pen.setWidthF(2.0);
  series->setPen(pen);

  double ymin = 1e300, ymax = -1e300;
  double xmin = samples.front().t, xmax = samples.back().t;
  for (const auto &s : samples) {
    const double y = (which == 0) ? s.Tc_out : s.Q;
    if (!std::isfinite(y)) continue;
    series->append(s.t, y);
    ymin = std::min(ymin, y);
    ymax = std::max(ymax, y);
  }

  chart->addSeries(series);
  const auto axes = chart->axes();
  for (auto *ax : axes) {
    series->attachAxis(ax);
    if (ax->orientation() == Qt::Horizontal) {
      auto *xa = qobject_cast<QValueAxis *>(ax);
      if (xa) {
        const double lo = std::min(xa->min(), xmin);
        const double hi = std::max(xa->max(), xmax);
        xa->setRange(lo, hi);
      }
    } else {
      auto *ya = qobject_cast<QValueAxis *>(ax);
      if (ya && std::isfinite(ymin) && std::isfinite(ymax)) {
        const double pad = 0.05 * std::max(1.0, ymax - ymin);
        const double lo = std::min(ya->min(), ymin - pad);
        const double hi = std::max(ya->max(), ymax + pad);
        ya->setRange(lo, hi);
      }
    }
  }
}

void clearChart(QChart *chart) {
  chart->removeAllSeries();
  for (auto *ax : chart->axes()) {
    if (auto *va = qobject_cast<QValueAxis *>(ax)) va->setRange(0.0, 1.0);
  }
}

} // anonymous namespace

RunLogDialog::RunLogDialog(QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("Run Log — History & Comparison"));
  resize(1180, 780);

  auto *main = new QVBoxLayout(this);

  // --- Header ---
  auto *header = new QLabel(this);
  header->setTextFormat(Qt::RichText);
  header->setText(QStringLiteral(
      "<div style='font-size:14pt;font-weight:700;color:#2c3e50;'>"
      "HeatXTwin Run History</div>"
      "<div style='color:#566573;font-size:9.5pt;'>"
      "All finished simulations are auto-saved to <code>%1</code>.  "
      "Select up to <b>two rows</b> (Ctrl-click) to overlay their time-series.</div>")
          .arg(hx::RunLog::instance().databasePath()));
  header->setStyleSheet("QLabel{background:#eaf4fb;padding:10px 14px;"
                         "border:1px solid #b5d4ea;border-radius:4px;}");
  main->addWidget(header);

  auto *split = new QSplitter(Qt::Vertical, this);
  main->addWidget(split, 1);

  // --- Top pane: table -----------------------------------------------------
  tbl_ = new QTableWidget(this);
  tbl_->setAlternatingRowColors(true);
  tbl_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tbl_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tbl_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tbl_->setSortingEnabled(true);
  tbl_->horizontalHeader()->setStretchLastSection(true);
  tbl_->verticalHeader()->setDefaultSectionSize(22);
  tbl_->verticalHeader()->setVisible(false);
  connect(tbl_, &QTableWidget::itemSelectionChanged, this, &RunLogDialog::onSelectionChanged);
  split->addWidget(tbl_);

  // --- Bottom pane: charts + summary ---------------------------------------
  auto *bottom = new QWidget(this);
  auto *bl = new QVBoxLayout(bottom);
  bl->setContentsMargins(0, 6, 0, 0);

  auto *chartRow = new QHBoxLayout();
  chartRow->setSpacing(6);
  chartTemp_ = makeChart(QStringLiteral("Tc_out(t)"),
                          QStringLiteral("Cold-side outlet"),
                          QStringLiteral("\xC2\xB0""C"));
  chartHeat_ = makeChart(QStringLiteral("Q(t)"),
                          QStringLiteral("Heat duty"),
                          QStringLiteral("W"));
  chartRow->addWidget(chartTemp_);
  chartRow->addWidget(chartHeat_);
  bl->addLayout(chartRow);

  lblSummary_ = new QLabel(this);
  lblSummary_->setWordWrap(true);
  lblSummary_->setTextFormat(Qt::RichText);
  lblSummary_->setStyleSheet("QLabel{background:#fdfefe;border:1px solid #d5dae0;"
                              "border-radius:4px;padding:8px 12px;"
                              "font-family:'Cambria Math',serif;color:#2c3e50;}");
  lblSummary_->setMinimumHeight(70);
  bl->addWidget(lblSummary_);

  split->addWidget(bottom);
  split->setStretchFactor(0, 3);
  split->setStretchFactor(1, 4);

  // --- Button row ----------------------------------------------------------
  auto *btnRow = new QHBoxLayout();
  btnRefresh_ = new QPushButton(QStringLiteral("Refresh"), this);
  btnDelete_  = new QPushButton(QStringLiteral("Delete selected"), this);
  btnRename_  = new QPushButton(QStringLiteral("Edit notes..."), this);
  btnExport_  = new QPushButton(QStringLiteral("Export samples to CSV"), this);
  for (auto *b : {btnRefresh_, btnDelete_, btnRename_, btnExport_})
    b->setMinimumHeight(30);
  btnDelete_->setStyleSheet("QPushButton{background:#c0392b;color:white;font-weight:600;}"
                             "QPushButton:hover{background:#e74c3c;}"
                             "QPushButton:disabled{background:#bfc9d1;}");
  btnRow->addWidget(btnRefresh_);
  btnRow->addStretch();
  btnRow->addWidget(btnRename_);
  btnRow->addWidget(btnExport_);
  btnRow->addWidget(btnDelete_);

  auto *close = new QPushButton(QStringLiteral("Close"), this);
  close->setMinimumHeight(30);
  btnRow->addWidget(close);
  main->addLayout(btnRow);

  connect(btnRefresh_, &QPushButton::clicked, this, &RunLogDialog::refresh);
  connect(btnDelete_,  &QPushButton::clicked, this, &RunLogDialog::onDeleteSelected);
  connect(btnRename_,  &QPushButton::clicked, this, &RunLogDialog::onRenameNote);
  connect(btnExport_,  &QPushButton::clicked, this, &RunLogDialog::onExportCsv);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);

  refresh();
}

void RunLogDialog::refresh() {
  rows_ = hx::RunLog::instance().listRuns();
  populateTable(rows_);
  lblSummary_->setText(QStringLiteral(
      "<i style='color:#566573;'>Select one row for KPI summary, "
      "or Ctrl-click a second row to compare two runs.</i>"));
  clearChart(chartTemp_->chart());
  clearChart(chartHeat_->chart());
}

void RunLogDialog::populateTable(const std::vector<hx::RunRecord> &rows) {
  tbl_->setSortingEnabled(false);
  tbl_->clearContents();
  const QStringList headers = {
      QStringLiteral("ID"), QStringLiteral("Started"),
      QStringLiteral("Duration [s]"), QStringLiteral("Mode"),
      QStringLiteral("Shell"), QStringLiteral("Arrangement"),
      QStringLiteral("Tc_out [\xC2\xB0""C]"), QStringLiteral("Th_out [\xC2\xB0""C]"),
      QStringLiteral("Q [kW]"), QStringLiteral("U [W/m\xC2\xB2""K]"),
      QStringLiteral("Rf [\xC3\x97""10\xE2\x81\xBB\xE2\x81\xB4]"),
      QStringLiteral("\xCE\x94""P_tube [kPa]"),
      QStringLiteral("\xCE\x94""P_shell [kPa]"),
      QStringLiteral("Notes"),
  };
  tbl_->setColumnCount(headers.size());
  tbl_->setHorizontalHeaderLabels(headers);
  tbl_->setRowCount(static_cast<int>(rows.size()));

  auto mkItem = [](const QString &s, bool num = false, double sortVal = 0.0) {
    auto *it = new QTableWidgetItem(s);
    if (num) {
      it->setData(Qt::UserRole, sortVal);
      it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    return it;
  };

  for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
    const auto &r = rows[static_cast<size_t>(i)];
    int col = 0;
    auto *idItem = mkItem(QString::number(r.id), true, static_cast<double>(r.id));
    idItem->setData(Qt::UserRole + 1, static_cast<qlonglong>(r.id));
    tbl_->setItem(i, col++, idItem);
    tbl_->setItem(i, col++, mkItem(r.started_at_iso));
    tbl_->setItem(i, col++, mkItem(QString::number(r.duration_s, 'f', 1), true, r.duration_s));
    tbl_->setItem(i, col++, mkItem(r.mode));
    tbl_->setItem(i, col++, mkItem(r.shell_method));
    tbl_->setItem(i, col++, mkItem(r.arrangement));
    tbl_->setItem(i, col++, mkItem(QString::number(r.Tc_out_final, 'f', 2), true, r.Tc_out_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.Th_out_final, 'f', 2), true, r.Th_out_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.Q_final / 1000.0, 'f', 2), true, r.Q_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.U_final, 'f', 1), true, r.U_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.Rf_final * 1.0e4, 'f', 3), true, r.Rf_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.dP_tube_final  / 1000.0, 'f', 2), true, r.dP_tube_final));
    tbl_->setItem(i, col++, mkItem(QString::number(r.dP_shell_final / 1000.0, 'f', 2), true, r.dP_shell_final));
    tbl_->setItem(i, col++, mkItem(r.notes));
  }

  tbl_->resizeColumnsToContents();
  tbl_->setSortingEnabled(true);
}

void RunLogDialog::onSelectionChanged() {
  updateComparison();
}

void RunLogDialog::updateComparison() {
  clearChart(chartTemp_->chart());
  clearChart(chartHeat_->chart());

  const auto selRows = tbl_->selectionModel()->selectedRows();
  if (selRows.isEmpty()) {
    lblSummary_->setText(QStringLiteral(
        "<i style='color:#566573;'>Select one row for KPI summary, "
        "or Ctrl-click a second row to compare two runs.</i>"));
    return;
  }

  // Up to two runs.  Colours: blue for first, orange for second.
  const QColor colors[2] = {QColor("#2c5784"), QColor("#d35400")};
  std::vector<hx::RunRecord> picked;
  for (int k = 0; k < selRows.size() && k < 2; ++k) {
    const int row = selRows[k].row();
    auto *idItem = tbl_->item(row, 0);
    if (!idItem) continue;
    const qint64 id = idItem->data(Qt::UserRole + 1).toLongLong();
    auto it = std::find_if(rows_.begin(), rows_.end(),
                           [id](const hx::RunRecord &r) { return r.id == id; });
    if (it == rows_.end()) continue;
    picked.push_back(*it);

    const auto samples = hx::RunLog::instance().loadSamples(id);
    plotRunOnChart(chartTemp_->chart(), *it, samples, 0, colors[k]);
    plotRunOnChart(chartHeat_->chart(), *it, samples, 1, colors[k]);
  }

  if (picked.empty()) { return; }

  QString html;
  if (picked.size() == 1) {
    const auto &r = picked[0];
    html = QStringLiteral(
        "<b>Run #%1</b> &mdash; %2 (%3)<br>"
        "T<sub>c,out</sub> = %4 &deg;C &nbsp;|&nbsp; "
        "T<sub>h,out</sub> = %5 &deg;C &nbsp;|&nbsp; "
        "Q = %6 kW &nbsp;|&nbsp; U = %7 W/m&sup2;K &nbsp;|&nbsp; "
        "R<sub>f</sub> = %8 &times;10<sup>-4</sup>")
            .arg(r.id).arg(r.mode, r.shell_method)
            .arg(fmt(r.Tc_out_final, 4)).arg(fmt(r.Th_out_final, 4))
            .arg(fmt(r.Q_final / 1000.0, 4)).arg(fmt(r.U_final, 4))
            .arg(fmt(r.Rf_final * 1.0e4, 4));
  } else {
    const auto &a = picked[0];
    const auto &b = picked[1];
    auto delta = [](double x, double y) {
      return QStringLiteral("%1 (%2%3%)")
          .arg(QString::number(y - x, 'f', 3))
          .arg((y - x) >= 0 ? QStringLiteral("+") : QStringLiteral(""))
          .arg(x > 1e-9 ? (100.0 * (y - x) / x) : 0.0, 0, 'f', 1);
    };
    html = QStringLiteral(
        "<b>Comparison:</b> #%1 vs #%2<br>"
        "&Delta;T<sub>c,out</sub> = %3 &deg;C &nbsp;|&nbsp; "
        "&Delta;Q = %4 kW &nbsp;|&nbsp; "
        "&Delta;U = %5 W/m&sup2;K &nbsp;|&nbsp; "
        "&Delta;R<sub>f</sub> = %6 &times;10<sup>-4</sup>")
            .arg(a.id).arg(b.id)
            .arg(delta(a.Tc_out_final, b.Tc_out_final))
            .arg(delta(a.Q_final / 1000.0, b.Q_final / 1000.0))
            .arg(delta(a.U_final, b.U_final))
            .arg(delta(a.Rf_final * 1.0e4, b.Rf_final * 1.0e4));
  }
  lblSummary_->setText(html);
}

void RunLogDialog::onDeleteSelected() {
  const auto selRows = tbl_->selectionModel()->selectedRows();
  if (selRows.isEmpty()) return;
  const auto reply = QMessageBox::question(this, QStringLiteral("Delete runs"),
      QStringLiteral("Permanently delete %1 selected run(s)?").arg(selRows.size()),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (reply != QMessageBox::Yes) return;

  std::vector<qint64> ids;
  for (const auto &idx : selRows) {
    auto *item = tbl_->item(idx.row(), 0);
    if (item) ids.push_back(item->data(Qt::UserRole + 1).toLongLong());
  }
  for (qint64 id : ids) hx::RunLog::instance().deleteRun(id);
  refresh();
}

void RunLogDialog::onRenameNote() {
  const auto selRows = tbl_->selectionModel()->selectedRows();
  if (selRows.size() != 1) {
    QMessageBox::information(this, QStringLiteral("Edit notes"),
        QStringLiteral("Select exactly one run first."));
    return;
  }
  auto *idItem = tbl_->item(selRows.first().row(), 0);
  if (!idItem) return;
  const qint64 id = idItem->data(Qt::UserRole + 1).toLongLong();
  auto it = std::find_if(rows_.begin(), rows_.end(),
                         [id](const hx::RunRecord &r) { return r.id == id; });
  if (it == rows_.end()) return;

  bool ok = false;
  const QString nv = QInputDialog::getMultiLineText(this,
      QStringLiteral("Run #%1 notes").arg(id),
      QStringLiteral("Notes (stored in SQLite):"),
      it->notes, &ok);
  if (!ok) return;
  if (hx::RunLog::instance().updateNotes(id, nv)) refresh();
}

void RunLogDialog::onExportCsv() {
  const auto selRows = tbl_->selectionModel()->selectedRows();
  if (selRows.size() != 1) {
    QMessageBox::information(this, QStringLiteral("Export CSV"),
        QStringLiteral("Select exactly one run first."));
    return;
  }
  auto *idItem = tbl_->item(selRows.first().row(), 0);
  if (!idItem) return;
  const qint64 id = idItem->data(Qt::UserRole + 1).toLongLong();
  const QString path = QFileDialog::getSaveFileName(this,
      QStringLiteral("Export run #%1 to CSV").arg(id),
      QStringLiteral("run_%1.csv").arg(id),
      QStringLiteral("CSV Files (*.csv)"));
  if (path.isEmpty()) return;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, QStringLiteral("Export CSV"),
        QStringLiteral("Could not open %1 for writing.").arg(path));
    return;
  }
  QTextStream out(&f);
  out << "t,Tc_out,Th_out,Q,U,Rf,dP_tube,dP_shell,pid_cmd\n";
  for (const auto &s : hx::RunLog::instance().loadSamples(id)) {
    out << s.t << "," << s.Tc_out << "," << s.Th_out << "," << s.Q << ","
        << s.U << "," << s.Rf << "," << s.dP_tube << "," << s.dP_shell << ","
        << s.pid_cmd << "\n";
  }
  f.close();
}
