#include "ChartWidget.hpp"
#include <QVBoxLayout>
#include <QPen>
#include <QColor>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <algorithm>

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
    
    // Update tooltip with nearest point
    QPointF chartPos = chart()->mapToValue(event->pos());
    
    // Build tooltip text by checking all series
    QString tooltipText;
    foreach (auto series, chart()->series()) {
      if (auto lineSeries = qobject_cast<QLineSeries*>(series)) {
        // Find closest point
        double minDist = 1e9;
        QPointF closestPoint;
        bool foundPoint = false;
        
        for (const QPointF& point : lineSeries->points()) {
          double dist = qAbs(point.x() - chartPos.x());
          if (dist < minDist) {
            minDist = dist;
            closestPoint = point;
            foundPoint = true;
          }
        }
        
        // If point is close enough (within 5% of x-axis range)
        if (foundPoint && minDist < (chart()->plotArea().width() * 0.05)) {
          if (!tooltipText.isEmpty()) tooltipText += "\n\n";
          tooltipText += QString("<b>%1</b><br/>Time: %2 s<br/>Value: %3")
                         .arg(lineSeries->name())
                         .arg(closestPoint.x(), 0, 'f', 2)
                         .arg(closestPoint.y(), 0, 'f', 3);
        }
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
      chart_->setTitle("🌡️ Temperature Profiles");
      axisY_->setTitleText("Temperature [°C]");
      axisY_->setRange(20, 100);
      minY_ = 20; maxY_ = 100;
      
      series1_ = createSeries("Tc_out (Cold Outlet)", QColor(41, 128, 185));   // Blue
      series2_ = createSeries("Th_out (Hot Outlet)", QColor(230, 126, 34));    // Orange
      series1Name_ = "Tc_out";
      series2Name_ = "Th_out";
      break;

    case HEAT_DUTY:
      chart_->setTitle("🔥 Heat Duty & Transfer Coefficient");
      axisY_->setTitleText("Q [kW] / U [W/m²K]");
      axisY_->setRange(0, 500);
      minY_ = 0; maxY_ = 500;
      
      series1_ = createSeries("Q (Heat Duty)", QColor(46, 204, 113));          // Green
      series2_ = createSeries("U (HT Coefficient)", QColor(155, 89, 182));     // Purple
      series1Name_ = "Q";
      series2Name_ = "U";
      break;

    case PRESSURE:
      chart_->setTitle("💧 Pressure Drops");
      axisY_->setTitleText("Pressure Drop [Pa]");
      axisY_->setRange(0, 40000);
      minY_ = 0; maxY_ = 40000;
      
      series1_ = createSeries("dP_tube (Tube Side)", QColor(26, 188, 156));    // Teal
      series2_ = createSeries("dP_shell (Shell Side)", QColor(231, 76, 60));   // Red
      series1Name_ = "dP_tube";
      series2Name_ = "dP_shell";
      break;

    case FOULING:
      chart_->setTitle("⚙️ Fouling Resistance");
      axisY_->setTitleText("Rf×10⁴ [m²K/W × 10⁴]");
      axisY_->setRange(0, 10);
      minY_ = 0; maxY_ = 10;
      
      series1_ = createSeries("Rf (Fouling)", QColor(52, 152, 219));           // Light Blue
      series1Name_ = "Rf";
      series2_ = nullptr;  // Only one series for fouling
      break;
  }
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
    case FOULING: axisY_->setRange(0, 10); break;
  }
  
  sampleCount_ = 0;
}

void ChartWidget::addSample(double t, const hx::State& state) {
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
  }

  double currentMin = std::min(val1, val2);
  double currentMax = std::max(val1, val2);
  
  // Adaptive range with 15% margin
  double range = currentMax - currentMin;
  double margin = std::max(range * 0.15, 1.0);  // At least 1 unit margin
  
  double newMin = std::max(minY_, currentMin - margin);
  double newMax = std::min(maxY_, currentMax + margin);
  
  // Only update if significant change
  if (std::abs(newMin - axisY_->min()) > range * 0.1 ||
      std::abs(newMax - axisY_->max()) > range * 0.1) {
    axisY_->setRange(newMin, newMax);
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
  }
}
