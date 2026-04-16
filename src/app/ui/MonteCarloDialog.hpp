#pragma once

#include <QDialog>
#include "core/MonteCarlo.hpp"

class QTableWidget;

/**
 * \brief Read-only Monte-Carlo results viewer.
 *
 *   Tab 1 — summary statistics (mean / stddev / percentiles) of all KPIs.
 *   Tab 2 — histograms (Q, U, ε).
 *   Tab 3 — tornado chart of ∂Q/∂param sensitivities.
 *
 * The dialog also offers CSV export of the raw per-trial vectors.
 */
class MonteCarloDialog : public QDialog {
  Q_OBJECT
public:
  explicit MonteCarloDialog(const hx::MonteCarloResult &result,
                             QWidget *parent = nullptr);

private slots:
  void onSaveCsv();

private:
  QWidget *buildStatsTab();
  QWidget *buildHistogramTab();
  QWidget *buildTornadoTab();

  hx::MonteCarloResult r_;
};
