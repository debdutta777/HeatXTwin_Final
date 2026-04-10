#include "HeatExchanger3DWidget.hpp"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/Qt3DWindow>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;

void addCylinderEntity(Qt3DCore::QEntity *parent,
                       float length,
                       float radius,
                       const QVector3D &translation,
                       const QQuaternion &rotation,
                       Qt3DExtras::QPhongMaterial *material) {
  auto *entity = new Qt3DCore::QEntity(parent);
  auto *mesh = new Qt3DExtras::QCylinderMesh(entity);
  mesh->setLength(length);
  mesh->setRadius(radius);
  mesh->setRings(32);
  mesh->setSlices(32);

  auto *transform = new Qt3DCore::QTransform(entity);
  transform->setTranslation(translation);
  transform->setRotation(rotation);

  entity->addComponent(mesh);
  entity->addComponent(transform);
  entity->addComponent(material);
}
}

HeatExchanger3DWidget::HeatExchanger3DWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  view_ = new Qt3DExtras::Qt3DWindow();
  container_ = QWidget::createWindowContainer(view_, this);
  layout->addWidget(container_);

  setupScene();
  setupLegend();
}

void HeatExchanger3DWidget::setupScene() {
  rootEntity_ = new Qt3DCore::QEntity();
  exchangerEntity_ = new Qt3DCore::QEntity(rootEntity_);
  exchangerTransform_ = new Qt3DCore::QTransform(exchangerEntity_);
  exchangerEntity_->addComponent(exchangerTransform_);

  camera_ = view_->camera();
  camera_->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.01f, 100.0f);
  camera_->setPosition(cameraHomePos_);
  camera_->setViewCenter(cameraHomeCenter_);

  auto *camController = new Qt3DExtras::QOrbitCameraController(rootEntity_);
  camController->setCamera(camera_);
  camController->setLinearSpeed(15.0f);
  camController->setLookSpeed(220.0f);

  auto *lightEntity = new Qt3DCore::QEntity(rootEntity_);
  auto *light = new Qt3DRender::QDirectionalLight(lightEntity);
  light->setColor(QColor(255, 255, 255));
  light->setIntensity(1.2f);
  light->setWorldDirection(QVector3D(-0.4f, -0.8f, -1.0f));
  lightEntity->addComponent(light);

  // Keep a fixed orientation so the model stays stable for inspection.
  exchangerTransform_->setRotation(QQuaternion::fromEulerAngles(12.0f, -18.0f, 0.0f));

  view_->setRootEntity(rootEntity_);
}

void HeatExchanger3DWidget::zoomIn() {
  if (!camera_) {
    return;
  }

  const QVector3D viewCenter = camera_->viewCenter();
  const QVector3D delta = camera_->position() - viewCenter;
  const float distance = delta.length();
  if (distance <= 0.0f) {
    return;
  }

  const float newDistance = std::max(0.45f, distance * 0.85f);
  camera_->setPosition(viewCenter + delta.normalized() * newDistance);
}

void HeatExchanger3DWidget::zoomOut() {
  if (!camera_) {
    return;
  }

  const QVector3D viewCenter = camera_->viewCenter();
  const QVector3D delta = camera_->position() - viewCenter;
  const float distance = delta.length();
  if (distance <= 0.0f) {
    return;
  }

  const float newDistance = std::min(12.0f, distance * 1.15f);
  camera_->setPosition(viewCenter + delta.normalized() * newDistance);
}

void HeatExchanger3DWidget::resetZoom() {
  if (!camera_) {
    return;
  }

  camera_->setPosition(cameraHomePos_);
  camera_->setViewCenter(cameraHomeCenter_);
}

void HeatExchanger3DWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (!legendBox_) {
    return;
  }

  const int margin = 14;
  legendBox_->adjustSize();
  legendBox_->move(width() - legendBox_->width() - margin, margin);
}

void HeatExchanger3DWidget::setupLegend() {
  legendBox_ = new QFrame(this);
  legendBox_->setStyleSheet(
    "QFrame {"
    "  background-color: rgba(255, 255, 255, 220);"
    "  border: 1px solid #b8c1cc;"
    "  border-radius: 8px;"
    "}"
    "QLabel {"
    "  color: #1f2937;"
    "  font-size: 9pt;"
    "}"
  );

  auto *legendLayout = new QVBoxLayout(legendBox_);
  legendLayout->setContentsMargins(10, 8, 10, 8);
  legendLayout->setSpacing(4);

  auto *title = new QLabel("3D Fluid Legend", legendBox_);
  title->setStyleSheet("font-weight: 700; color: #0f172a;");
  legendLayout->addWidget(title);

  auto *hotLine = new QLabel("● Hot Fluid (Tube Side)", legendBox_);
  hotLine->setStyleSheet("color: #e4572e; font-weight: 600;");
  legendLayout->addWidget(hotLine);

  auto *coldLine = new QLabel("● Cold Fluid (Shell Side)", legendBox_);
  coldLine->setStyleSheet("color: #1f78ff; font-weight: 600;");
  legendLayout->addWidget(coldLine);

  legendBox_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  legendBox_->adjustSize();
  legendBox_->raise();
}

void HeatExchanger3DWidget::setGeometryData(const hx::Geometry &geometry) {
  geometry_ = geometry;
  hasGeometry_ = true;
  rebuildExchangerGeometry();
}

void HeatExchanger3DWidget::updateSimulationState(const hx::State &state) {
  if (tubeMaterials_.isEmpty()) {
    return;
  }

  const double tc = state.Tc_out;
  const double th = state.Th_out;
  const double span = std::max(1.0, th - tc);
  const double foulingLevel = std::clamp(state.Rf / 0.01, 0.0, 1.0);

  const QColor coldColor(36, 123, 255);
  const QColor hotColor(245, 94, 40);
  const QColor fouledTint(112, 73, 45);

  const int n = tubeMaterials_.size();
  for (int i = 0; i < n; ++i) {
    const double localT = tc + span * (static_cast<double>(i) / std::max(1, n - 1));
    const double norm = std::clamp((localT - tc) / std::max(0.1, span), 0.0, 1.0);
    const QColor base = lerpColor(coldColor, hotColor, norm);
    const QColor finalColor = lerpColor(base, fouledTint, foulingLevel * 0.55);
    tubeMaterials_[i]->setDiffuse(finalColor);
  }

  if (shellMaterial_) {
    const int gray = static_cast<int>(std::clamp(190.0 - foulingLevel * 65.0, 120.0, 210.0));
    shellMaterial_->setDiffuse(QColor(gray, gray, gray, 120));
  }

  if (coldFluidMaterial_) {
    const QColor coldFluidBase = lerpColor(QColor(40, 122, 255), QColor(24, 92, 214), foulingLevel * 0.35);
    coldFluidMaterial_->setDiffuse(coldFluidBase);
  }
}

void HeatExchanger3DWidget::rebuildExchangerGeometry() {
  if (!hasGeometry_ || !rootEntity_) {
    return;
  }

  // Guard checks: ensure geometry is valid before rebuilding
  if (geometry_.nTubes < 1 || geometry_.nTubes > 5000) {
    return;  // Invalid tube count
  }
  
  if (geometry_.Di <= 0.0 || geometry_.Do <= 0.0 || geometry_.L <= 0.0) {
    return;  // Invalid tube dimensions
  }
  
  if (geometry_.Do <= geometry_.Di) {
    return;  // Invalid diameter relationship
  }
  
  if (geometry_.shellID <= geometry_.Do) {
    return;  // Shell too small for tubes
  }

  // Safe to rebuild
  delete exchangerEntity_;
  exchangerEntity_ = new Qt3DCore::QEntity(rootEntity_);
  exchangerTransform_ = new Qt3DCore::QTransform(exchangerEntity_);
  exchangerEntity_->addComponent(exchangerTransform_);

  tubeMaterials_.clear();
  shellMaterial_ = nullptr;
  coldFluidMaterial_ = nullptr;

  addShell();
  addTubeBundle();
  addBaffles();
  addNozzles();
}

void HeatExchanger3DWidget::addShell() {
  shellMaterial_ = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  shellMaterial_->setDiffuse(QColor(188, 188, 188, 120));
  shellMaterial_->setSpecular(QColor(230, 230, 230));
  shellMaterial_->setShininess(35.0f);

  const float length = static_cast<float>(std::max(1.0, geometry_.L));
  const float radius = static_cast<float>(std::max(0.05, geometry_.shellID * 0.5));

  // Qt cylinder axis is Y, rotate to align exchanger along X.
  const QQuaternion rotateToX = QQuaternion::fromEulerAngles(0.0f, 0.0f, 90.0f);
  addCylinderEntity(exchangerEntity_, length, radius, QVector3D(0.0f, 0.0f, 0.0f), rotateToX, shellMaterial_);

  // Cold shell-side fluid body (blue) to visually differentiate fluid type.
  coldFluidMaterial_ = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  coldFluidMaterial_->setDiffuse(QColor(40, 122, 255));
  coldFluidMaterial_->setSpecular(QColor(160, 210, 255));
  coldFluidMaterial_->setShininess(14.0f);
  addCylinderEntity(exchangerEntity_, length * 0.96f, radius * 0.82f, QVector3D(0.0f, 0.0f, 0.0f), rotateToX, coldFluidMaterial_);

  auto *endCapMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  endCapMaterial->setDiffuse(QColor(160, 160, 160));

  auto *leftCap = new Qt3DCore::QEntity(exchangerEntity_);
  auto *leftCapMesh = new Qt3DExtras::QSphereMesh(leftCap);
  leftCapMesh->setRadius(radius * 0.35f);
  auto *leftCapTransform = new Qt3DCore::QTransform(leftCap);
  leftCapTransform->setTranslation(QVector3D(-length * 0.5f, 0.0f, 0.0f));
  leftCap->addComponent(leftCapMesh);
  leftCap->addComponent(leftCapTransform);
  leftCap->addComponent(endCapMaterial);

  auto *rightCap = new Qt3DCore::QEntity(exchangerEntity_);
  auto *rightCapMesh = new Qt3DExtras::QSphereMesh(rightCap);
  rightCapMesh->setRadius(radius * 0.35f);
  auto *rightCapTransform = new Qt3DCore::QTransform(rightCap);
  rightCapTransform->setTranslation(QVector3D(length * 0.5f, 0.0f, 0.0f));
  rightCap->addComponent(rightCapMesh);
  rightCap->addComponent(rightCapTransform);
  rightCap->addComponent(endCapMaterial);
}

void HeatExchanger3DWidget::addTubeBundle() {
  const int target = std::max(1, geometry_.nTubes);
  const double shellRadius = std::max(0.05, geometry_.shellID * 0.5);
  const double tubeRadius = std::max(0.002, geometry_.Do * 0.5);
  const float tubeLength = static_cast<float>(std::max(0.8, geometry_.L * 0.97));
  const double minPitch = geometry_.Do * 1.2;

  const QQuaternion rotateToX = QQuaternion::fromEulerAngles(0.0f, 0.0f, 90.0f);

  double pitch = std::max(geometry_.pitch, minPitch);
  std::vector<QVector3D> positions;
  positions.reserve(static_cast<size_t>(target));

  for (int attempt = 0; attempt < 6 && static_cast<int>(positions.size()) < target; ++attempt) {
    positions.clear();
    const int gridSize = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(target)))) + 2;
    const double safeRadius = shellRadius - tubeRadius * 2.0;

    for (int i = -gridSize / 2; i <= gridSize / 2 && static_cast<int>(positions.size()) < target; ++i) {
      for (int j = -gridSize / 2; j <= gridSize / 2 && static_cast<int>(positions.size()) < target; ++j) {
        const double y = i * pitch;
        const double z = j * pitch;
        const double distFromCenter = std::sqrt(y * y + z * z);

        if (distFromCenter + tubeRadius < safeRadius) {
          bool overlaps = false;
          for (const QVector3D &existing : positions) {
            const double dy = y - existing.y();
            const double dz = z - existing.z();
            const double distBetween = std::sqrt(dy * dy + dz * dz);
            if (distBetween < tubeRadius * 2.1) {
              overlaps = true;
              break;
            }
          }
          if (!overlaps) {
            positions.emplace_back(0.0f, static_cast<float>(y), static_cast<float>(z));
          }
        }
      }
    }

    if (static_cast<int>(positions.size()) < target) {
      pitch *= 0.85;
      if (pitch < minPitch * 0.7) {
        pitch = minPitch * 0.7;
      }
    }
  }

  auto *tubeMaterialParent = new Qt3DCore::QEntity(exchangerEntity_);
  for (const QVector3D &pos : positions) {
    auto *material = new Qt3DExtras::QPhongMaterial(tubeMaterialParent);
    material->setDiffuse(QColor(240, 95, 48));
    material->setSpecular(QColor(245, 245, 245));
    material->setShininess(20.0f);

    addCylinderEntity(exchangerEntity_, tubeLength, static_cast<float>(tubeRadius), pos, rotateToX, material);
    tubeMaterials_.push_back(material);
  }
}

void HeatExchanger3DWidget::addBaffles() {
  auto *baffleMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  baffleMaterial->setDiffuse(QColor(100, 140, 180));
  baffleMaterial->setSpecular(QColor(180, 200, 220));
  baffleMaterial->setShininess(32.0f);

  const int nBaffles = std::max(0, geometry_.nBaffles);
  const double spacing = std::max(0.08, geometry_.baffleSpacing);
  const double shellRadius = std::max(0.05, geometry_.shellID * 0.48);

  for (int i = 0; i < nBaffles; ++i) {
    const double x = -geometry_.L * 0.5 + spacing * (i + 1);
    if (x >= geometry_.L * 0.5) {
      break;
    }

    auto *baffle = new Qt3DCore::QEntity(exchangerEntity_);
    auto *mesh = new Qt3DExtras::QCylinderMesh(baffle);
    mesh->setLength(0.02f);
    mesh->setRadius(static_cast<float>(shellRadius));
    mesh->setRings(8);
    mesh->setSlices(32);

    auto *transform = new Qt3DCore::QTransform(baffle);
    transform->setRotation(QQuaternion::fromEulerAngles(0.0f, 0.0f, 90.0f));
    transform->setTranslation(QVector3D(static_cast<float>(x), 0.0f, 0.0f));

    baffle->addComponent(mesh);
    baffle->addComponent(transform);
    baffle->addComponent(baffleMaterial);
  }
}

void HeatExchanger3DWidget::addNozzles() {
  auto *hotMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  hotMaterial->setDiffuse(QColor(220, 96, 72));
  hotMaterial->setSpecular(QColor(240, 150, 130));
  hotMaterial->setShininess(30.0f);

  auto *coldMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  coldMaterial->setDiffuse(QColor(77, 151, 255));
  coldMaterial->setSpecular(QColor(140, 190, 255));
  coldMaterial->setShininess(30.0f);

  auto *flangeMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  flangeMaterial->setDiffuse(QColor(180, 180, 185));
  flangeMaterial->setSpecular(QColor(220, 220, 220));
  flangeMaterial->setShininess(40.0f);

  const float nozzleLength = 0.35f;
  const float shellRadius = static_cast<float>(std::max(0.05, geometry_.shellID * 0.5));
  const float halfLength = static_cast<float>(std::max(1.0, geometry_.L) * 0.5);
  const float hotNozzleRadius = 0.052f;
  const float coldNozzleRadius = 0.068f;

  // Hot inlet nozzle at left end-cap
  const QVector3D hotInletPos(-halfLength * 0.95f, 0.0f, shellRadius + nozzleLength * 0.30f);
  addCylinderEntity(exchangerEntity_, nozzleLength, hotNozzleRadius, hotInletPos,
                    QQuaternion::fromEulerAngles(90.0f, 0.0f, 0.0f), hotMaterial);
  addCylinderEntity(exchangerEntity_, 0.015f, hotNozzleRadius * 1.3f,
                    QVector3D(hotInletPos.x(), hotInletPos.y(), hotInletPos.z() - nozzleLength * 0.5f),
                    QQuaternion::fromEulerAngles(90.0f, 0.0f, 0.0f), flangeMaterial);

  // Hot outlet nozzle at right end-cap
  const QVector3D hotOutletPos(halfLength * 0.95f, 0.0f, -(shellRadius + nozzleLength * 0.30f));
  addCylinderEntity(exchangerEntity_, nozzleLength, hotNozzleRadius, hotOutletPos,
                    QQuaternion::fromEulerAngles(90.0f, 0.0f, 0.0f), hotMaterial);
  addCylinderEntity(exchangerEntity_, 0.015f, hotNozzleRadius * 1.3f,
                    QVector3D(hotOutletPos.x(), hotOutletPos.y(), hotOutletPos.z() + nozzleLength * 0.5f),
                    QQuaternion::fromEulerAngles(90.0f, 0.0f, 0.0f), flangeMaterial);

  // Cold inlet nozzle at top mid-length
  const QVector3D coldInletPos(0.0f, shellRadius + nozzleLength * 0.30f, 0.0f);
  addCylinderEntity(exchangerEntity_, nozzleLength, coldNozzleRadius, coldInletPos,
                    QQuaternion::fromEulerAngles(0.0f, 0.0f, 0.0f), coldMaterial);
  addCylinderEntity(exchangerEntity_, 0.015f, coldNozzleRadius * 1.3f,
                    QVector3D(coldInletPos.x(), coldInletPos.y() - nozzleLength * 0.5f, coldInletPos.z()),
                    QQuaternion::fromEulerAngles(0.0f, 0.0f, 0.0f), flangeMaterial);

  // Cold outlet nozzle at bottom mid-length
  const QVector3D coldOutletPos(0.0f, -(shellRadius + nozzleLength * 0.30f), 0.0f);
  addCylinderEntity(exchangerEntity_, nozzleLength, coldNozzleRadius, coldOutletPos,
                    QQuaternion::fromEulerAngles(0.0f, 0.0f, 0.0f), coldMaterial);
  addCylinderEntity(exchangerEntity_, 0.015f, coldNozzleRadius * 1.3f,
                    QVector3D(coldOutletPos.x(), coldOutletPos.y() + nozzleLength * 0.5f, coldOutletPos.z()),
                    QQuaternion::fromEulerAngles(0.0f, 0.0f, 0.0f), flangeMaterial);

  auto *arrowMaterial = new Qt3DExtras::QPhongMaterial(exchangerEntity_);
  arrowMaterial->setDiffuse(QColor(255, 214, 102));
  arrowMaterial->setSpecular(QColor(255, 235, 160));
  arrowMaterial->setShininess(25.0f);

  auto *cone1 = new Qt3DCore::QEntity(exchangerEntity_);
  auto *coneMesh1 = new Qt3DExtras::QConeMesh(cone1);
  coneMesh1->setTopRadius(0.0f);
  coneMesh1->setBottomRadius(0.03f);
  coneMesh1->setLength(0.10f);
  auto *coneTf1 = new Qt3DCore::QTransform(cone1);
  coneTf1->setRotation(QQuaternion::fromEulerAngles(0.0f, 0.0f, -90.0f));
  coneTf1->setTranslation(QVector3D(-halfLength * 0.2f, 0.0f, shellRadius * 0.7f));
  cone1->addComponent(coneMesh1);
  cone1->addComponent(coneTf1);
  cone1->addComponent(arrowMaterial);

  auto *cone2 = new Qt3DCore::QEntity(exchangerEntity_);
  auto *coneMesh2 = new Qt3DExtras::QConeMesh(cone2);
  coneMesh2->setTopRadius(0.0f);
  coneMesh2->setBottomRadius(0.03f);
  coneMesh2->setLength(0.10f);
  auto *coneTf2 = new Qt3DCore::QTransform(cone2);
  coneTf2->setRotation(QQuaternion::fromEulerAngles(0.0f, 0.0f, 90.0f));
  coneTf2->setTranslation(QVector3D(halfLength * 0.2f, 0.0f, -shellRadius * 0.7f));
  cone2->addComponent(coneMesh2);
  cone2->addComponent(coneTf2);
  cone2->addComponent(arrowMaterial);
}

QColor HeatExchanger3DWidget::lerpColor(const QColor &a, const QColor &b, double t) {
  t = std::clamp(t, 0.0, 1.0);
  const int r = static_cast<int>(a.red() + (b.red() - a.red()) * t);
  const int g = static_cast<int>(a.green() + (b.green() - a.green()) * t);
  const int bl = static_cast<int>(a.blue() + (b.blue() - a.blue()) * t);
  return QColor(r, g, bl);
}
