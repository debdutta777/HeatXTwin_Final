#include "MonteCarloDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QLocale>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <cmath>

namespace {

QString fmtNum(double v, int prec = 3) {
  if (!std::isfinite(v)) return QStringLiteral("—");
  return QLocale::c().toString(v, 'g', prec);
}

struct HistogramBins {
  std::vector<double> centers;
  std::vector<int>    counts;
  double lo = 0.0;
  double hi = 0.0;
};

HistogramBins makeHistogram(const std::vector<double> &data, int nBins) {
  HistogramBins h;
  if (data.empty()) return h;
  double lo = data.front();
  double hi = data.front();
  for (double x : data) { lo = std::min(lo, x); hi = std::max(hi, x); }
  if (std::fabs(hi - lo) < 1e-12) {
    // Degenerate: single value — give it a narrow symmetric band.
    const double pad = (std::fabs(lo) > 1e-9) ? 0.01 * std::fabs(lo) : 1.0;
    lo -= pad; hi += pad;
  }
  const double w = (hi - lo) / static_cast<double>(nBins);
  h.lo = lo; h.hi = hi;
  h.counts.assign(static_cast<size_t>(nBins), 0);
  h.centers.resize(static_cast<size_t>(nBins));
  for (int i = 0; i < nBins; ++i) {
    h.centers[static_cast<size_t>(i)] = lo + (i + 0.5) * w;
  }
  for (double x : data) {
    int idx = static_cast<int>(std::floor((x - lo) / w));
    if (idx < 0) idx = 0;
    if (idx >= nBins) idx = nBins - 1;
    h.counts[static_cast<size_t>(idx)]++;
  }
  return h;
}

QChart *buildHistogramChart(const QString &title,
                            const QString &xUnit,
                            const std::vector<double> &data,
                            const hx::MonteCarloStat  &stat) {
  auto *chart = new QChart();
  chart->setTitle(title);
  chart->legend()->hide();
  chart->setAnimationOptions(QChart::NoAnimation);

  constexpr int kBins = 28;
  const HistogramBins h = makeHistogram(data, kBins);

  auto *set = new QBarSet(QObject::tr("count"));
  set->setColor(QColor("#3498db"));
  set->setBorderColor(QColor("#2c3e50"));
  int maxCount = 0;
  for (int c : h.counts) { set->append(c); maxCount = std::max(maxCount, c); }

  auto *series = new QBarSeries();
  series->append(set);
  series->setBarWidth(0.98);
  chart->addSeries(series);

  QStringList categories;
  categories.reserve(kBins);
  for (size_t i = 0; i < h.centers.size(); ++i) {
    categories.append(QString::number(h.centers[i], 'g', 3));
  }

  auto *axX = new QBarCategoryAxis();
  axX->append(categories);
  axX->setTitleText(xUnit);
  // Only show every ~5th label to keep it readable.
  axX->setLabelsAngle(-60);
  chart->addAxis(axX, Qt::AlignBottom);
  series->attachAxis(axX);

  auto *axY = new QValueAxis();
  axY->setTitleText(QObject::tr("trials"));
  axY->setRange(0, static_cast<double>(maxCount) * 1.15 + 1);
  axY->setLabelFormat("%d");
  chart->addAxis(axY, Qt::AlignLeft);
  series->attachAxis(axY);

  // Mean & ±1σ annotation via the title.
  chart->setTitle(QStringLiteral("%1  —  μ = %2, σ = %3, p5..p95 = %4..%5")
                      .arg(title,
                           fmtNum(stat.mean, 4),
                           fmtNum(stat.stddev, 3),
                           fmtNum(stat.p5, 4),
                           fmtNum(stat.p95, 4)));
  return chart;
}

} // namespace

MonteCarloDialog::MonteCarloDialog(const hx::MonteCarloResult &result,
                                     QWidget *parent)
    : QDialog(parent), r_(result) {
  setWindowTitle(QStringLiteral("Monte-Carlo Sensitivity — %1 trials").arg(r_.nTrials));
  resize(1100, 720);

  auto *mainLayout = new QVBoxLayout(this);

  auto *summary = new QLabel(QString::fromStdString(r_.message), this);
  summary->setWordWrap(true);
  summary->setStyleSheet(QStringLiteral(
      "QLabel{background:#f4f8fb;border-left:4px solid #3498db;"
      "padding:8px 12px;color:#2c3e50;font-size:10pt;}"));
  mainLayout->addWidget(summary);

  auto *tabs = new QTabWidget(this);
  tabs->addTab(buildStatsTab(),     QStringLiteral("Summary statistics"));
  tabs->addTab(buildHistogramTab(), QStringLiteral("Distributions"));
  tabs->addTab(buildTornadoTab(),   QStringLiteral("Tornado"));
  mainLayout->addWidget(tabs, 1);

  auto *buttons = new QHBoxLayout();
  auto *csvBtn  = new QPushButton(QStringLiteral("Save CSV..."), this);
  auto *close   = new QPushButton(QStringLiteral("Close"), this);
  buttons->addWidget(csvBtn);
  buttons->addStretch(1);
  buttons->addWidget(close);
  mainLayout->addLayout(buttons);

  connect(csvBtn, &QPushButton::clicked, this, &MonteCarloDialog::onSaveCsv);
  connect(close,  &QPushButton::clicked, this, &QDialog::accept);
}

QWidget *MonteCarloDialog::buildStatsTab() {
  auto *w  = new QWidget(this);
  auto *vb = new QVBoxLayout(w);

  auto *table = new QTableWidget(w);
  const QStringList rows    = {QStringLiteral("Heat duty Q [W]"),
                                QStringLiteral("Overall U [W/m²K]"),
                                QStringLiteral("Tc,out [°C]"),
                                QStringLiteral("Effectiveness ε [-]"),
                                QStringLiteral("ΔP tube [Pa]"),
                                QStringLiteral("ΔP shell [Pa]")};
  const QStringList columns = {QStringLiteral("Mean"),
                                QStringLiteral("Std-dev"),
                                QStringLiteral("Min"),
                                QStringLiteral("p5"),
                                QStringLiteral("p50"),
                                QStringLiteral("p95"),
                                QStringLiteral("Max")};

  table->setRowCount(rows.size());
  table->setColumnCount(columns.size());
  table->setHorizontalHeaderLabels(columns);
  table->setVerticalHeaderLabels(rows);
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table->verticalHeader()->setDefaultSectionSize(28);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setAlternatingRowColors(true);

  const hx::MonteCarloStat *stats[] = {
      &r_.statQ, &r_.statU, &r_.statTc, &r_.statEps, &r_.statDPt, &r_.statDPs};
  for (int i = 0; i < rows.size(); ++i) {
    const auto &s = *stats[i];
    const double vals[] = {s.mean, s.stddev, s.minv, s.p5, s.p50, s.p95, s.maxv};
    for (int j = 0; j < columns.size(); ++j) {
      auto *item = new QTableWidgetItem(fmtNum(vals[j], 5));
      item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
      table->setItem(i, j, item);
    }
  }
  vb->addWidget(table);

  auto *note = new QLabel(
      QStringLiteral("<i>%1 Monte-Carlo trials with σ applied to flows, "
                     "inlet temperatures, fluid properties%2. Baseline heat "
                     "duty Q₀ = %3 W, baseline U₀ = %4 W/m²·K.</i>")
          .arg(r_.nTrials)
          .arg(r_.tornado.empty() ? QStringLiteral("")
                                   : QStringLiteral(", and fouling asymptote"))
          .arg(fmtNum(r_.baselineQ, 5))
          .arg(fmtNum(r_.baselineU, 5)),
      w);
  note->setWordWrap(true);
  note->setStyleSheet(QStringLiteral("color:#566573;padding-top:6px;"));
  vb->addWidget(note);
  return w;
}

QWidget *MonteCarloDialog::buildHistogramTab() {
  auto *w = new QWidget(this);
  auto *vb = new QVBoxLayout(w);

  auto *splitter = new QSplitter(Qt::Vertical, w);

  auto *chartQ = buildHistogramChart(QStringLiteral("Heat duty Q"),
                                      QStringLiteral("Q [W]"),
                                      r_.Q, r_.statQ);
  auto *viewQ  = new QChartView(chartQ, splitter);
  viewQ->setRenderHint(QPainter::Antialiasing);
  splitter->addWidget(viewQ);

  auto *chartU = buildHistogramChart(QStringLiteral("Overall heat transfer coefficient U"),
                                      QStringLiteral("U [W/m²K]"),
                                      r_.U, r_.statU);
  auto *viewU  = new QChartView(chartU, splitter);
  viewU->setRenderHint(QPainter::Antialiasing);
  splitter->addWidget(viewU);

  auto *chartE = buildHistogramChart(QStringLiteral("Effectiveness ε"),
                                      QStringLiteral("ε [-]"),
                                      r_.eps, r_.statEps);
  auto *viewE  = new QChartView(chartE, splitter);
  viewE->setRenderHint(QPainter::Antialiasing);
  splitter->addWidget(viewE);

  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 1);
  splitter->setStretchFactor(2, 1);

  vb->addWidget(splitter);
  return w;
}

QWidget *MonteCarloDialog::buildTornadoTab() {
  auto *w  = new QWidget(this);
  auto *vb = new QVBoxLayout(w);

  auto *chart = new QChart();
  chart->setTitle(QStringLiteral(
      "Sensitivity of heat duty Q to ±1σ perturbation of each input "
      "(sorted by magnitude; baseline Q₀ = %1 W)").arg(fmtNum(r_.baselineQ, 4)));
  chart->setAnimationOptions(QChart::NoAnimation);

  auto *setPlus  = new QBarSet(QStringLiteral("+1σ"));
  auto *setMinus = new QBarSet(QStringLiteral("−1σ"));
  setPlus ->setColor(QColor("#e74c3c"));
  setMinus->setColor(QColor("#2980b9"));
  setPlus ->setBorderColor(QColor("#7f1d1d"));
  setMinus->setBorderColor(QColor("#1f3b5f"));

  QStringList categories;
  // Plot from least to most sensitive so the biggest bar sits at the top.
  for (auto it = r_.tornado.rbegin(); it != r_.tornado.rend(); ++it) {
    categories.append(QString::fromStdString(it->name));
    setPlus ->append(it->deltaPlus);
    setMinus->append(it->deltaMinus);
  }

  auto *series = new QHorizontalBarSeries();
  series->append(setMinus);
  series->append(setPlus);
  series->setBarWidth(0.75);
  chart->addSeries(series);

  auto *axY = new QBarCategoryAxis();
  axY->append(categories);
  chart->addAxis(axY, Qt::AlignLeft);
  series->attachAxis(axY);

  // Symmetric X axis around 0 for a proper tornado look.
  double mx = 0.0;
  for (const auto &e : r_.tornado) {
    mx = std::max(mx, std::fabs(e.deltaPlus));
    mx = std::max(mx, std::fabs(e.deltaMinus));
  }
  if (mx < 1e-9) mx = 1.0;
  auto *axX = new QValueAxis();
  axX->setRange(-1.15 * mx, 1.15 * mx);
  axX->setTitleText(QStringLiteral("ΔQ from baseline [W]"));
  axX->setLabelFormat("%.0f");
  chart->addAxis(axX, Qt::AlignBottom);
  series->attachAxis(axX);

  auto *view = new QChartView(chart, w);
  view->setRenderHint(QPainter::Antialiasing);
  vb->addWidget(view);

  auto *note = new QLabel(
      QStringLiteral("<i>Each row shows ΔQ when the named parameter is "
                     "displaced by one standard deviation (all other "
                     "parameters held at nominal). Longer bars ⇒ "
                     "heat-transfer performance is more sensitive to that "
                     "input.</i>"), w);
  note->setWordWrap(true);
  note->setStyleSheet(QStringLiteral("color:#566573;padding:4px 8px;"));
  vb->addWidget(note);
  return w;
}

void MonteCarloDialog::onSaveCsv() {
  const QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("Save Monte-Carlo Data"),
      QStringLiteral("HeatXTwin_MonteCarlo.csv"),
      QStringLiteral("CSV Files (*.csv)"));
  if (path.isEmpty()) return;

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(this, QStringLiteral("Error"),
                          QStringLiteral("Could not open file for writing."));
    return;
  }
  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  out << "trial,Q_W,U_Wm2K,Tc_out_C,Th_out_C,dP_tube_Pa,dP_shell_Pa,eps,NTU\n";
  for (int i = 0; i < r_.nTrials; ++i) {
    out << i
        << ',' << r_.Q[i]
        << ',' << r_.U[i]
        << ',' << r_.Tc_out[i]
        << ',' << r_.Th_out[i]
        << ',' << r_.dP_tube[i]
        << ',' << r_.dP_shell[i]
        << ',' << r_.eps[i]
        << ',' << r_.NTU[i]
        << '\n';
  }
  out << "\n# Tornado sensitivity (ΔQ at ±1σ per parameter):\n";
  out << "# parameter,deltaPlus_W,deltaMinus_W,sensitivity_W\n";
  for (const auto &e : r_.tornado) {
    out << "# " << QString::fromStdString(e.name)
        << ',' << e.deltaPlus
        << ',' << e.deltaMinus
        << ',' << e.sensitivity
        << '\n';
  }
  file.close();
}
