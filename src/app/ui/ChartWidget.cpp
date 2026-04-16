#include "ChartWidget.hpp"
#include <QVBoxLayout>
#include <QPen>
#include <QColor>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

// ============================================================================
// CustomChartView Implementation
// ============================================================================

CustomChartView::CustomChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent) {
  setRenderHint(QPainter::Antialiasing);
  setRubberBand(QChartView::RectangleRubberBand);
  setInteractive(true);
  setFocusPolicy(Qt::StrongFocus);  // Enable keyboard events
  setMouseTracking(true);  // Enable mouse tracking for tooltips
}

void CustomChartView::wheelEvent(QWheelEvent *event) {
  // Mouse wheel zoom
  qreal factor = event->angleDelta().y() > 0 ? 0.9 : 1.1;  // Zoom in/out
  chart()->zoom(factor);
  event->accept();
}

void CustomChartView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Space && !m_isPanning) {
    m_isPanning = true;
    setCursor(Qt::OpenHandCursor);
    event->accept();
  } else {
    QChartView::keyPressEvent(event);
  }
}

void CustomChartView::keyReleaseEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Space) {
    m_isPanning = false;
    setCursor(Qt::ArrowCursor);
    event->accept();
  } else {
    QChartView::keyReleaseEvent(event);
  }
}

void CustomChartView::mousePressEvent(QMouseEvent *event) {
  if (m_isPanning && event->button() == Qt::LeftButton) {
    m_lastPanPoint = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
  } else {
    QChartView::mousePressEvent(event);
  }
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event) {
  if (m_isPanning && (event->buttons() & Qt::LeftButton)) {
    auto delta = event->pos() - m_lastPanPoint;
    chart()->scroll(-delta.x(), delta.y());
    m_lastPanPoint = event->pos();
    event->accept();
  } else {
    // Pass to base class for hover events
    QChartView::mouseMoveEvent(event);
    
    // Update tooltip with nearest point.  Samples are monotonically
    // increasing in t, so binary-search once per series instead of an O(n)
    // linear scan of a by-value copy of the full point list.
    const QPointF chartPos = chart()->mapToValue(event->pos());

    QString tooltipText;
    const auto allSeries = chart()->series();
    for (auto *series : allSeries) {
      auto *lineSeries = qobject_cast<QLineSeries*>(series);
      if (!lineSeries) continue;
      const int n = lineSeries->count();
      if (n == 0) continue;

      const double xq = chartPos.x();
      int lo = 0, hi = n - 1;
      while (lo < hi) {
        const int mid = (lo + hi) >> 1;
        if (lineSeries->at(mid).x() < xq) lo = mid + 1;
        else                              hi = mid;
      }
      // lo is the first index whose x ≥ xq; check it and its left neighbour.
      QPointF closest = lineSeries->at(lo);
      double minDist = std::abs(closest.x() - xq);
      if (lo > 0) {
        const QPointF left = lineSeries->at(lo - 1);
        const double dL = std::abs(left.x() - xq);
        if (dL < minDist) { closest = left; minDist = dL; }
      }

      if (minDist < chart()->plotArea().width() * 0.05) {
        if (!tooltipText.isEmpty()) tooltipText += "\n\n";
        tooltipText += QString("<b>%1</b><br/>Time: %2 s<br/>Value: %3")
                       .arg(lineSeries->name())
                       .arg(closest.x(), 0, 'f', 2)
                       .arg(closest.y(), 0, 'f', 3);
      }
    }

    if (!tooltipText.isEmpty()) {
      setToolTip(tooltipText);
    }
  }
}

void CustomChartView::mouseReleaseEvent(QMouseEvent *event) {
  if (m_isPanning && event->button() == Qt::LeftButton) {
    setCursor(Qt::OpenHandCursor);
    event->accept();
  } else {
    QChartView::mouseReleaseEvent(event);
  }
}

// ============================================================================
// ChartWidget Implementation
// ============================================================================

ChartWidget::ChartWidget(ChartType type, QWidget *parent)
    : QWidget(parent), type_(type) {
  setupChart();
  setupSeries();
  
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(chartView_);
}

void ChartWidget::setupChart() {
  chart_ = new QChart();
  chart_->legend()->setVisible(true);
  chart_->legend()->setAlignment(Qt::AlignBottom);
  chart_->setAnimationOptions(QChart::SeriesAnimations);
  
  // Modern styling
  chart_->setBackgroundBrush(QBrush(QColor(250, 250, 250)));
  chart_->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
  
  chartView_ = new CustomChartView(chart_, this);
  chartView_->setBackgroundBrush(QBrush(QColor(245, 245, 245)));
  chartView_->setMouseTracking(true);  // Enable mouse tracking for hover
  
  // Axes
  axisX_ = new QValueAxis();
  axisX_->setTitleText("Time [seconds]");
  axisX_->setRange(0, 100);
  axisX_->setLabelsColor(QColor(60, 60, 60));
  
  axisY_ = new QValueAxis();
  axisY_->setLabelsColor(QColor(60, 60, 60));
  
  chart_->addAxis(axisX_, Qt::AlignBottom);
  chart_->addAxis(axisY_, Qt::AlignLeft);
}

void ChartWidget::setupSeries() {
  auto createSeries = [&](const QString& name, const QColor& color) {
    auto *series = new QLineSeries();
    series->setName(name);
    QPen pen(color);
    pen.setWidth(2);
    series->setPen(pen);
    chart_->addSeries(series);
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
    
    // Enable point labeling (optional - shows on hover)
    series->setPointLabelsVisible(false);
    
    return series;
  };

  switch (type_) {
    case TEMPERATURE:
      chart_->setTitle("Temperature Profiles");
      axisY_->setTitleText("Temperature [\xC2\xB0""C]");
      axisY_->setRange(20, 100);
      minY_ = 20; maxY_ = 100;

      series1_ = createSeries("Tc,out (Cold Outlet)", QColor(41, 128, 185));   // Blue
      series2_ = createSeries("Th,out (Hot Outlet)", QColor(230, 126, 34));    // Orange
      series1Name_ = "Tc out";
      series2Name_ = "Th out";
      break;

    case HEAT_DUTY:
      chart_->setTitle("Heat Duty & Overall Heat Transfer Coefficient");
      axisY_->setTitleText("Q [kW] / U [W/m\xC2\xB2\xC2\xB7K]");
      axisY_->setRange(0, 500);
      minY_ = 0; maxY_ = 500;

      series1_ = createSeries("Q - Heat Duty [kW]", QColor(46, 204, 113));          // Green
      series2_ = createSeries("U - HT Coefficient [W/m\xC2\xB2\xC2\xB7K]", QColor(155, 89, 182));  // Purple
      series1Name_ = "Q";
      series2Name_ = "U";
      break;

    case PRESSURE:
      chart_->setTitle("Pressure Drop Analysis");
      axisY_->setTitleText("Pressure Drop [Pa]");
      axisY_->setRange(0, 40000);
      minY_ = 0; maxY_ = 40000;

      series1_ = createSeries("Tube Side dP [Pa]", QColor(26, 188, 156));    // Teal
      series2_ = createSeries("Shell Side dP [Pa]", QColor(231, 76, 60));    // Red
      series1Name_ = "dP tube";
      series2Name_ = "dP shell";
      break;

    case FOULING:
      chart_->setTitle("Fouling Resistance Growth");
      axisY_->setTitleText("Rf x 10\xE2\x81\xB4 [m\xC2\xB2\xC2\xB7K/W]");
      axisY_->setRange(0, 20);
      minY_ = 0; maxY_ = 100; // Allow up to 0.01 Rf

      series1_ = createSeries("Rf - Fouling Resistance", QColor(52, 152, 219));  // Light Blue
      series1Name_ = "Rf";
      series2_ = nullptr;  // Only one series for fouling
      break;

    case PID_CONTROL:
      chart_->setTitle("PID Controller Tracking");
      axisY_->setTitleText("Temperature [\xC2\xB0""C]");
      axisY_->setRange(0, 120);
      minY_ = 0; maxY_ = 150;

      // Create left-axis series (temperature)
      series1_ = createSeries("Tc,out [\xC2\xB0""C]", QColor(41, 128, 185));
      series2_ = createSeries("Setpoint [\xC2\xB0""C]", QColor(231, 76, 60));
      series1Name_ = "Tc out";
      series2Name_ = "Setpoint";

      // Create right-axis series (flow rate)
      axisY2_ = new QValueAxis();
      axisY2_->setTitleText("Cold Flow Rate [kg/s]");
      axisY2_->setRange(0, 10);
      axisY2_->setLabelsColor(QColor(60, 60, 60));
      chart_->addAxis(axisY2_, Qt::AlignRight);
      minY2_ = 0; maxY2_ = 10;

      series3_ = new QLineSeries();
      series3_->setName("Cold Flow [kg/s]");
      QPen pen3(QColor(46, 204, 113));
      pen3.setWidth(2);
      series3_->setPen(pen3);
      chart_->addSeries(series3_);
      series3_->attachAxis(axisX_);
      series3_->attachAxis(axisY2_);  // Attach to right axis
      series3_->setPointLabelsVisible(false);
      series3Name_ = "Cold Flow";
      break;
  }
}

namespace {
// Render a dashed version of a colour at 60% alpha for baseline overlays.
QPen baselinePen(const QColor &c) {
  QColor fade = c;
  fade.setAlpha(140);
  QPen pen(fade);
  pen.setWidth(2);
  pen.setStyle(Qt::DashLine);
  return pen;
}

QLineSeries *copyAsBaseline(QLineSeries *src, const QString &suffix) {
  if (!src) return nullptr;
  auto *copy = new QLineSeries();
  copy->setName(src->name() + suffix);
  copy->setPen(baselinePen(src->pen().color()));
  copy->replace(src->points());
  return copy;
}
}  // namespace

void ChartWidget::captureBaseline() {
  clearBaseline();

  auto attach = [&](QLineSeries *s, QValueAxis *yAx) {
    if (!s || s->count() == 0) return static_cast<QLineSeries*>(nullptr);
    auto *b = copyAsBaseline(s, " (baseline)");
    chart_->addSeries(b);
    b->attachAxis(axisX_);
    b->attachAxis(yAx);
    return b;
  };

  baseline1_ = attach(series1_, axisY_);
  baseline2_ = attach(series2_, axisY_);
  baseline3_ = attach(series3_, type_ == PID_CONTROL && axisY2_ ? axisY2_ : axisY_);

  hasBaseline_ = (baseline1_ || baseline2_ || baseline3_);
}

void ChartWidget::clearBaseline() {
  auto drop = [&](QLineSeries *&b) {
    if (!b) return;
    chart_->removeSeries(b);
    delete b;
    b = nullptr;
  };
  drop(baseline1_);
  drop(baseline2_);
  drop(baseline3_);
  hasBaseline_ = false;
}

void ChartWidget::clear() {
  series1_->clear();
  if (series2_) series2_->clear();
  if (series3_) series3_->clear();
  
  axisX_->setRange(0, 100);
  
  // Reset Y range based on chart type
  switch (type_) {
    case TEMPERATURE: axisY_->setRange(20, 100); break;
    case HEAT_DUTY: axisY_->setRange(0, 500); break;
    case PRESSURE: axisY_->setRange(0, 40000); break;
    case FOULING: axisY_->setRange(0, 20); break;
    case PID_CONTROL:
      axisY_->setRange(0, 120);
      if (axisY2_) axisY2_->setRange(0, 10);
      break;
  }
  
  sampleCount_ = 0;
  flowSeenMin_ =  std::numeric_limits<double>::infinity();
  flowSeenMax_ = -std::numeric_limits<double>::infinity();
}

void ChartWidget::addSample(double t, const hx::State& state, double pidSetpointTcOut, double coldFlow) {
  // Add data points based on chart type
  switch (type_) {
    case TEMPERATURE:
      series1_->append(t, state.Tc_out);
      series2_->append(t, state.Th_out);
      break;

    case HEAT_DUTY:
      series1_->append(t, state.Q / 1000.0);  // Watts to kW
      series2_->append(t, state.U);
      break;

    case PRESSURE:
      series1_->append(t, state.dP_tube);
      series2_->append(t, state.dP_shell);
      break;

    case FOULING:
      series1_->append(t, state.Rf * 10000.0);  // Scale for visibility
      break;

    case PID_CONTROL:
      series1_->append(t, state.Tc_out);
      if (series2_) {
        const double setpoint = std::isnan(pidSetpointTcOut) ? state.Tc_out : pidSetpointTcOut;
        series2_->append(t, setpoint);
      }
      if (series3_) {
        const double flow = std::isnan(coldFlow) ? 0.0 : coldFlow;  // No scaling
        series3_->append(t, flow);
        // Update running min/max incrementally so updateAxes() doesn't
        // have to copy+scan the entire QLineSeries on each tick.
        if (flow < flowSeenMin_) flowSeenMin_ = flow;
        if (flow > flowSeenMax_) flowSeenMax_ = flow;
      }
      break;
  }

  // Auto-scale time axis
  if (t > axisX_->max()) {
    axisX_->setMax(t + 10);
  }

  // Auto-scale Y axis with proper margins
  if (++sampleCount_ % 10 == 0) {  // Update every 10 samples
    updateAxes(t, state);
  }
}

void ChartWidget::updateAxes(double t, const hx::State& state) {
  (void)t;
  double val1 = 0, val2 = 0;
  
  switch (type_) {
    case TEMPERATURE:
      val1 = state.Tc_out;
      val2 = state.Th_out;
      break;
    case HEAT_DUTY:
      val1 = state.Q / 1000.0;
      val2 = state.U;
      break;
    case PRESSURE:
      val1 = state.dP_tube;
      val2 = state.dP_shell;
      break;
    case FOULING:
      val1 = state.Rf * 10000.0;
      val2 = val1;
      break;
    case PID_CONTROL:
      val1 = state.Tc_out;
      val2 = state.Tc_out;
      break;
  }

  double currentMin = std::min(val1, val2);
  double currentMax = std::max(val1, val2);
  
  // Adaptive range with 15% margin
  double range = currentMax - currentMin;
  double margin = std::max(range * 0.15, 1.0);  // At least 1 unit margin
  
  // For fouling, ensure a minimum display range to prevent noise amplification
  if (type_ == FOULING) {
      margin = std::max(margin, 0.5);
  }

  double newMin = std::max(minY_, currentMin - margin);
  double newMax = std::min(maxY_, currentMax + margin);
  
  if (type_ == FOULING) {
      // Ensure we show at least 0-5 range (scaled) if values are small
      if (newMax < 5.0) {
          newMax = 5.0;
      }
  }
  
  // Only update if significant change
  if (std::abs(newMin - axisY_->min()) > range * 0.1 ||
      std::abs(newMax - axisY_->max()) > range * 0.1) {
    axisY_->setRange(newMin, newMax);
  }
  
  // Update right Y-axis for PID control chart with flow rate data.
  // Use the running min/max maintained in addSample() so we don't have
  // to copy & scan series3_->points() (O(n)) every time.
  if (type_ == PID_CONTROL && axisY2_ && series3_ && series3_->count() > 0 &&
      std::isfinite(flowSeenMin_) && std::isfinite(flowSeenMax_)) {
    const double flowMin = flowSeenMin_;
    const double flowMax = flowSeenMax_;
    const double flowRange = flowMax - flowMin;
    const double flowMargin = std::max(flowRange * 0.15, 0.1);
    const double newFlowMin = std::max(minY2_, flowMin - flowMargin);
    const double newFlowMax = std::min(maxY2_, flowMax + flowMargin);

    if (std::abs(newFlowMin - axisY2_->min()) > flowRange * 0.1 ||
        std::abs(newFlowMax - axisY2_->max()) > flowRange * 0.1) {
      axisY2_->setRange(newFlowMin, newFlowMax);
    }
  }
}

void ChartWidget::zoomIn() {
  chart_->zoomIn();
}

void ChartWidget::zoomOut() {
  chart_->zoomOut();
}

void ChartWidget::resetZoom() {
  chart_->zoomReset();
  // Reset to default ranges
  axisX_->setRange(0, series1_->count() > 0 ? series1_->at(series1_->count()-1).x() : 100);
  
  switch (type_) {
    case TEMPERATURE: axisY_->setRange(20, 100); break;
    case HEAT_DUTY: axisY_->setRange(0, 500); break;
    case PRESSURE: axisY_->setRange(0, 40000); break;
    case FOULING: axisY_->setRange(0, 10); break;
    case PID_CONTROL:
      axisY_->setRange(0, 120);
      if (axisY2_) axisY2_->setRange(0, 10);
      break;
  }
}
