#pragma once

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QSlider>
#include <QLabel>

#include "core/Types.hpp"

class HeatExchangerWidget : public QWidget {
  Q_OBJECT

public:
  explicit HeatExchangerWidget(QWidget *parent = nullptr);

  void setGeometryData(const hx::Geometry &geometry);
  void updateSimulationState(const hx::State &state);
  void setOperatingPoint(const hx::OperatingPoint &op);
  void setSimulationRunning(bool running);

public slots:
  void zoomIn();
  void zoomOut();
  void resetZoom();

signals:
  void hotFlowRateChanged(double value);
  void coldFlowRateChanged(double value);

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void onAnimationTick();
  void onHotSliderChanged(int value);
  void onColdSliderChanged(int value);

private:
  // --- Drawing helpers ---
  struct DrawRect {
    double x, y, w, h;
  };
  DrawRect shellRect() const;

  void drawBackground(QPainter &p);
  void drawShell(QPainter &p);
  void drawTubes(QPainter &p);
  void drawBaffles(QPainter &p);
  void drawHeaders(QPainter &p);
  void drawNozzles(QPainter &p);
  void drawParticles(QPainter &p);
  void drawFlowArrows(QPainter &p);
  void drawLabels(QPainter &p);
  void drawLegend(QPainter &p);
  void drawFoulingLayer(QPainter &p);
  void drawDataOverlay(QPainter &p);
  void drawSupports(QPainter &p);

  // --- Particle system ---
  struct Particle {
    double x, y;       // position in widget coords
    double progress;    // 0..1 along path
    double speed;       // step per tick
    int pathIndex;      // which tube or shell path
    bool isTubeSide;    // true = hot/tube, false = cold/shell
  };

  void initParticles();
  void updateParticles();
  QPointF tubePath(int tubeIndex, double t) const;
  QPointF shellPath(int pathIndex, double t) const;

  // --- Color helpers ---
  static QColor lerpColor(const QColor &a, const QColor &b, double t);
  QColor hotColor(double t) const;   // t=0 inlet, t=1 outlet
  QColor coldColor(double t) const;

  // --- Setup ---
  void setupControls();

  // --- State ---
  hx::Geometry geom_{};
  hx::State state_{};
  hx::OperatingPoint op_{};
  bool hasGeometry_{false};
  bool hasState_{false};
  bool simulationRunning_{false};

  // --- Animation ---
  QTimer *animTimer_{};
  QVector<Particle> particles_;
  double animPhase_{0.0};
  int particleCount_{120};

  // --- View ---
  double zoomLevel_{1.0};

  // --- Simulation values for overlay ---
  double foulingFraction_{0.0};   // 0..1 normalized fouling level

  // --- Layout constants (recomputed on resize) ---
  double margin_{60.0};
  double shellX_{}, shellY_{}, shellW_{}, shellH_{};
  double headerW_{};
  int visibleTubes_{9};

  // --- Flow controls ---
  QWidget *controlPanel_{};
  QSlider *hotFlowSlider_{};
  QSlider *coldFlowSlider_{};
  QLabel *hotFlowLabel_{};
  QLabel *coldFlowLabel_{};
};
