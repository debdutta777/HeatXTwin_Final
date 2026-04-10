#pragma once

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <vector>
#include <limits>
#include "core/Types.hpp"

/**
 * @brief Custom ChartView with mouse wheel zoom and space-bar panning
 */
class CustomChartView : public QChartView {
  Q_OBJECT
public:
  explicit CustomChartView(QChart *chart, QWidget *parent = nullptr);

protected:
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  bool m_isPanning{false};
  QPoint m_lastPanPoint;
};

/**
 * @brief Single optimized chart widget for specific data types
 * 
 * Each chart shows only related series with appropriate scaling.
 * Much clearer than cramming all 7 series on one chart!
 */
class ChartWidget : public QWidget {
  Q_OBJECT

public:
  enum ChartType {
    TEMPERATURE,  // Tc_out, Th_out (20-100°C)
    HEAT_DUTY,    // Q, U (0-500)
    PRESSURE,     // dP_tube, dP_shell (0-40k)
    FOULING,      // Rf (0-10)
    PID_CONTROL   // Tc_out, setpoint, cold flow (scaled)
  };

  explicit ChartWidget(ChartType type, QWidget *parent = nullptr);
  
  void clear();
  void addSample(double t, const hx::State& state, double pidSetpointTcOut = std::numeric_limits<double>::quiet_NaN(), double coldFlow = std::numeric_limits<double>::quiet_NaN());
  
public slots:
  void zoomIn();
  void zoomOut();
  void resetZoom();

private:
  void setupChart();
  void setupSeries();
  void updateAxes(double t, const hx::State& state);

  ChartType type_;
  QChart *chart_{};
  CustomChartView *chartView_{};
  QValueAxis *axisX_{};
  QValueAxis *axisY_{};
  QValueAxis *axisY2_{};  // Right Y-axis for dual-axis PID chart
  
  // Series (only 2-3 per chart for clarity)
  QLineSeries *series1_{};
  QLineSeries *series2_{};
  QLineSeries *series3_{};  // Optional
  
  QString series1Name_;
  QString series2Name_;
  QString series3Name_;
  
  double minY_{0.0};
  double maxY_{100.0};
  double minY2_{0.0};  // Right Y-axis min for PID
  double maxY2_{10.0}; // Right Y-axis max for PID
  int sampleCount_{0};
};
