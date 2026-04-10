#pragma once

#include <QWidget>
#include <QVector>
#include <QVector3D>

#include "core/Types.hpp"

class QFrame;
class QResizeEvent;

namespace Qt3DCore {
class QEntity;
class QTransform;
}

namespace Qt3DRender {
class QCamera;
}

namespace Qt3DExtras {
class Qt3DWindow;
class QPhongMaterial;
class QOrbitCameraController;
}

class HeatExchanger3DWidget : public QWidget {
  Q_OBJECT

public:
  explicit HeatExchanger3DWidget(QWidget *parent = nullptr);

  void setGeometryData(const hx::Geometry &geometry);
  void updateSimulationState(const hx::State &state);

public slots:
  void zoomIn();
  void zoomOut();
  void resetZoom();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void setupScene();
  void setupLegend();
  void rebuildExchangerGeometry();
  void addShell();
  void addTubeBundle();
  void addBaffles();
  void addNozzles();
  static QColor lerpColor(const QColor &a, const QColor &b, double t);

  hx::Geometry geometry_{};
  bool hasGeometry_{false};

  Qt3DExtras::Qt3DWindow *view_{};
  QWidget *container_{};

  Qt3DCore::QEntity *rootEntity_{};
  Qt3DCore::QEntity *exchangerEntity_{};
  Qt3DCore::QTransform *exchangerTransform_{};
  Qt3DRender::QCamera *camera_{};
  QFrame *legendBox_{};

  Qt3DExtras::QPhongMaterial *shellMaterial_{};
  Qt3DExtras::QPhongMaterial *coldFluidMaterial_{};
  QVector<Qt3DExtras::QPhongMaterial *> tubeMaterials_;

  QVector3D cameraHomePos_{0.0f, -2.0f, 1.4f};
  QVector3D cameraHomeCenter_{0.0f, 0.0f, 0.0f};
};
