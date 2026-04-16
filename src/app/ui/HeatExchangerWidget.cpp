#include "HeatExchangerWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QFont>
#include <QFontMetrics>

#include <algorithm>
#include <cmath>
#include <random>

namespace {
constexpr double kPi = 3.14159265358979323846;

// Blueprint palette
const QColor kBgDark(18, 32, 58);
const QColor kBgGrid(30, 50, 85);
const QColor kGridLine(40, 65, 110, 60);

// Component colors
const QColor kShellOuter(160, 170, 180);
const QColor kShellInner(130, 140, 150);
const QColor kBaffleColor(100, 150, 200);
const QColor kHeaderColor(170, 175, 182);
const QColor kSupportColor(120, 125, 130);
const QColor kFlangeColor(190, 195, 200);

// Fluid colors
const QColor kHotInlet(240, 60, 30);       // bright red
const QColor kHotOutlet(255, 160, 60);     // orange
const QColor kColdInlet(30, 120, 255);     // bright blue
const QColor kColdOutlet(100, 200, 255);   // light cyan

// Fouling
const QColor kFoulingColor(140, 95, 50, 180);

// Labels
const QColor kLabelColor(220, 230, 245);
const QColor kLabelBg(20, 40, 70, 200);

// Nozzle
const QColor kNozzleHot(200, 75, 50);
const QColor kNozzleCold(50, 100, 220);

} // namespace

// ============================================================================
// Construction
// ============================================================================

HeatExchangerWidget::HeatExchangerWidget(QWidget *parent)
    : QWidget(parent) {
  setMinimumSize(600, 350);

  // Default operating point
  op_.m_dot_hot = 1.2;
  op_.m_dot_cold = 1.0;
  op_.Tin_hot = 80.0;
  op_.Tin_cold = 25.0;

  // Setup animation timer (~30 fps, only repaints — particles gated by simulationRunning_)
  animTimer_ = new QTimer(this);
  animTimer_->setInterval(33);
  connect(animTimer_, &QTimer::timeout, this, &HeatExchangerWidget::onAnimationTick);
  animTimer_->start();

  setupControls();
  initParticles();
}

void HeatExchangerWidget::setupControls() {
  // Overlay control panel at bottom
  controlPanel_ = new QWidget(this);
  controlPanel_->setStyleSheet(
    "QWidget { background: rgba(15, 28, 50, 210); border-top: 2px solid #3498db; }"
    "QLabel { color: #c0d0e8; font-size: 9pt; font-weight: 600; background: transparent; border: none; }"
    "QSlider::groove:horizontal { height: 6px; background: #1a3050; border-radius: 3px; }"
    "QSlider::handle:horizontal { width: 16px; height: 16px; margin: -5px 0;"
    "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.3,"
    "  stop:0 #5dade2, stop:1 #2980b9); border-radius: 8px; }"
    "QSlider::sub-page:horizontal { background: #2980b9; border-radius: 3px; }"
  );

  auto *cl = new QHBoxLayout(controlPanel_);
  cl->setContentsMargins(20, 6, 20, 6);
  cl->setSpacing(12);

  // Hot flow control
  auto *hotIcon = new QLabel("Hot Flow:", controlPanel_);
  hotIcon->setStyleSheet("color: #e74c3c; font-weight: bold; background: transparent; border: none;");
  cl->addWidget(hotIcon);

  hotFlowSlider_ = new QSlider(Qt::Horizontal, controlPanel_);
  hotFlowSlider_->setRange(10, 500);  // 0.1 to 5.0 kg/s * 100
  hotFlowSlider_->setValue(static_cast<int>(op_.m_dot_hot * 100));
  hotFlowSlider_->setMinimumWidth(120);
  connect(hotFlowSlider_, &QSlider::valueChanged, this, &HeatExchangerWidget::onHotSliderChanged);
  cl->addWidget(hotFlowSlider_);

  hotFlowLabel_ = new QLabel(QString("%1 kg/s").arg(op_.m_dot_hot, 0, 'f', 1), controlPanel_);
  hotFlowLabel_->setMinimumWidth(65);
  cl->addWidget(hotFlowLabel_);

  cl->addSpacing(20);

  // Cold flow control
  auto *coldIcon = new QLabel("Cold Flow:", controlPanel_);
  coldIcon->setStyleSheet("color: #3498db; font-weight: bold; background: transparent; border: none;");
  cl->addWidget(coldIcon);

  coldFlowSlider_ = new QSlider(Qt::Horizontal, controlPanel_);
  coldFlowSlider_->setRange(10, 500);
  coldFlowSlider_->setValue(static_cast<int>(op_.m_dot_cold * 100));
  coldFlowSlider_->setMinimumWidth(120);
  connect(coldFlowSlider_, &QSlider::valueChanged, this, &HeatExchangerWidget::onColdSliderChanged);
  cl->addWidget(coldFlowSlider_);

  coldFlowLabel_ = new QLabel(QString("%1 kg/s").arg(op_.m_dot_cold, 0, 'f', 1), controlPanel_);
  coldFlowLabel_->setMinimumWidth(65);
  cl->addWidget(coldFlowLabel_);

  cl->addStretch();
}

// ============================================================================
// Public interface
// ============================================================================

void HeatExchangerWidget::setGeometryData(const hx::Geometry &geometry) {
  geom_ = geometry;
  hasGeometry_ = true;
  initParticles();
  update();
}

void HeatExchangerWidget::updateSimulationState(const hx::State &state) {
  state_ = state;
  hasState_ = true;
  foulingFraction_ = std::clamp(state.Rf / 0.005, 0.0, 1.0);
  update();
}

void HeatExchangerWidget::setOperatingPoint(const hx::OperatingPoint &op) {
  op_ = op;
  if (hotFlowSlider_) {
    QSignalBlocker b1(hotFlowSlider_);
    hotFlowSlider_->setValue(static_cast<int>(op.m_dot_hot * 100));
    hotFlowLabel_->setText(QString("%1 kg/s").arg(op.m_dot_hot, 0, 'f', 1));
  }
  if (coldFlowSlider_) {
    QSignalBlocker b2(coldFlowSlider_);
    coldFlowSlider_->setValue(static_cast<int>(op.m_dot_cold * 100));
    coldFlowLabel_->setText(QString("%1 kg/s").arg(op.m_dot_cold, 0, 'f', 1));
  }
  initParticles();
  update();
}

void HeatExchangerWidget::setSimulationRunning(bool running) {
  simulationRunning_ = running;
  update();
}

void HeatExchangerWidget::zoomIn() {
  zoomLevel_ = std::min(2.5, zoomLevel_ * 1.15);
  update();
}

void HeatExchangerWidget::zoomOut() {
  zoomLevel_ = std::max(0.5, zoomLevel_ / 1.15);
  update();
}

void HeatExchangerWidget::resetZoom() {
  zoomLevel_ = 1.0;
  update();
}

// ============================================================================
// Slots
// ============================================================================

void HeatExchangerWidget::onAnimationTick() {
  if (!simulationRunning_) {
    // Still repaint for static display, but don't move particles
    update();
    return;
  }
  animPhase_ += 0.033;
  if (animPhase_ > 1000.0) animPhase_ = 0.0;
  updateParticles();
  update();
}

void HeatExchangerWidget::onHotSliderChanged(int value) {
  double flowRate = value / 100.0;
  op_.m_dot_hot = flowRate;
  hotFlowLabel_->setText(QString("%1 kg/s").arg(flowRate, 0, 'f', 1));
  emit hotFlowRateChanged(flowRate);
}

void HeatExchangerWidget::onColdSliderChanged(int value) {
  double flowRate = value / 100.0;
  op_.m_dot_cold = flowRate;
  coldFlowLabel_->setText(QString("%1 kg/s").arg(flowRate, 0, 'f', 1));
  emit coldFlowRateChanged(flowRate);
}

// ============================================================================
// Layout
// ============================================================================

void HeatExchangerWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  int ctrlH = 44;
  if (controlPanel_) {
    controlPanel_->setGeometry(0, height() - ctrlH, width(), ctrlH);
    controlPanel_->raise();
  }

  double availW = width() * zoomLevel_;
  double availH = (height() - ctrlH) * zoomLevel_;
  margin_ = std::max(50.0, std::min(availW, availH) * 0.08);

  headerW_ = std::max(40.0, availW * 0.06);
  shellX_ = margin_ + headerW_;
  shellY_ = margin_ + 40;
  shellW_ = availW - 2 * margin_ - 2 * headerW_;
  shellH_ = availH - 2 * margin_ - 80;

  if (shellW_ < 100) shellW_ = 100;
  if (shellH_ < 60)  shellH_ = 60;
}

HeatExchangerWidget::DrawRect HeatExchangerWidget::shellRect() const {
  return {shellX_, shellY_, shellW_, shellH_};
}

// ============================================================================
// Particle system
// ============================================================================

void HeatExchangerWidget::initParticles() {
  particles_.clear();
  particles_.reserve(particleCount_);

  int hotCount = particleCount_ * 3 / 5;
  int coldCount = particleCount_ - hotCount;

  // Hot/tube-side particles — evenly distributed across tubes with staggered start
  for (int i = 0; i < hotCount; ++i) {
    Particle p;
    p.pathIndex = i % visibleTubes_;
    // Evenly space particles along each tube so they form a visible stream
    int particlesPerTube = std::max(1, hotCount / visibleTubes_);
    int indexInTube = i / visibleTubes_;
    p.progress = static_cast<double>(indexInTube) / particlesPerTube;
    p.isTubeSide = true;
    p.speed = 0.0005;  // uniform speed for clean flow
    p.x = p.y = 0;
    particles_.push_back(p);
  }

  // Cold/shell-side particles — 6 streams, evenly spaced along zigzag path
  int coldStreams = 6;
  for (int i = 0; i < coldCount; ++i) {
    Particle p;
    p.pathIndex = i % coldStreams;
    // Evenly distribute along the full path
    int particlesPerStream = std::max(1, coldCount / coldStreams);
    int indexInStream = i / coldStreams;
    p.progress = static_cast<double>(indexInStream) / particlesPerStream;
    p.isTubeSide = false;
    p.speed = 0.0004;  // uniform speed for clean flow
    p.x = p.y = 0;
    particles_.push_back(p);
  }
}

void HeatExchangerWidget::updateParticles() {
  double hotSpeedMul = std::clamp(op_.m_dot_hot / 1.2, 0.2, 3.0);
  double coldSpeedMul = std::clamp(op_.m_dot_cold / 1.0, 0.2, 3.0);

  for (auto &p : particles_) {
    double mul = p.isTubeSide ? hotSpeedMul : coldSpeedMul;
    p.progress += p.speed * mul;
    if (p.progress >= 1.0) {
      p.progress -= 1.0;
    }

    if (p.isTubeSide) {
      QPointF pt = tubePath(p.pathIndex, p.progress);
      p.x = pt.x();
      p.y = pt.y();
    } else {
      QPointF pt = shellPath(p.pathIndex, p.progress);
      p.x = pt.x();
      p.y = pt.y();
    }
  }
}

QPointF HeatExchangerWidget::tubePath(int tubeIndex, double t) const {
  // Exact same coordinates as drawTubes() — particles stay inside tubes
  double tubeSpacing = shellH_ / (visibleTubes_ + 1);
  double cy = shellY_ + tubeSpacing * (tubeIndex + 1);
  double tubeStartX = shellX_ + 6.0;
  double tubeLen = shellW_ - 12.0;

  // Flow left to right along tube centerline (no wobble)
  double x = tubeStartX + t * tubeLen;
  return QPointF(x, cy);
}

QPointF HeatExchangerWidget::shellPath(int pathIndex, double t) const {
  // Shell-side fluid flows through baffle gaps in organized zigzag.
  // Path: enter from cold inlet nozzle (top), zigzag down-up between baffles,
  // exit through cold outlet nozzle (bottom).
  int nBaffles = std::max(3, hasGeometry_ ? geom_.nBaffles : 8);

  // Each segment: horizontal travel between two baffles + vertical transition
  // Total path has (nBaffles + 1) horizontal segments and nBaffles vertical transitions
  // We model it as alternating horizontal and vertical segments.
  int totalSegments = 2 * nBaffles + 1;  // H, V, H, V, H, ...

  // Shell interior bounds (stay inside shell wall)
  double innerLeft = shellX_ + 8.0;
  double innerRight = shellX_ + shellW_ - 8.0;
  double innerTop = shellY_ + shellH_ * 0.10;
  double innerBot = shellY_ + shellH_ * 0.90;

  // Baffle positions
  double baffleSpacing = shellW_ / (nBaffles + 1);

  // Slight vertical offset for each of the 4 particle streams
  double streamOffset = (pathIndex - 1.5) * shellH_ * 0.04;

  // Determine which segment and local progress
  double scaled = t * totalSegments;
  int segIdx = static_cast<int>(scaled) % totalSegments;
  double localT = scaled - std::floor(scaled);

  // Compute baffle column index: horizontal segment i corresponds to gap i/2
  int hSegIdx = segIdx / 2;  // which horizontal gap (0 to nBaffles)

  // Boundaries of current horizontal gap
  double gapLeft = (hSegIdx == 0) ? innerLeft : (shellX_ + baffleSpacing * hSegIdx + 3.0);
  double gapRight = (hSegIdx == nBaffles) ? innerRight : (shellX_ + baffleSpacing * (hSegIdx + 1) - 3.0);

  // Vertical position at this gap — alternates top/bottom for each gap
  double yPos;
  if (hSegIdx % 2 == 0) {
    yPos = innerTop + streamOffset;  // flow near top
  } else {
    yPos = innerBot + streamOffset;  // flow near bottom
  }

  if (segIdx % 2 == 0) {
    // Horizontal segment: move from gapLeft to gapRight
    double x = gapLeft + localT * (gapRight - gapLeft);
    return QPointF(x, yPos);
  } else {
    // Vertical segment: transition at baffle from current yPos to next yPos
    int nextHSeg = hSegIdx + 1;
    double yNext;
    if (nextHSeg % 2 == 0) {
      yNext = innerTop + streamOffset;
    } else {
      yNext = innerBot + streamOffset;
    }
    double baffleX = shellX_ + baffleSpacing * (hSegIdx + 1);
    double y = yPos + localT * (yNext - yPos);
    return QPointF(baffleX, y);
  }
}

// ============================================================================
// Color helpers
// ============================================================================

QColor HeatExchangerWidget::lerpColor(const QColor &a, const QColor &b, double t) {
  t = std::clamp(t, 0.0, 1.0);
  return QColor(
    static_cast<int>(a.red()   + (b.red()   - a.red())   * t),
    static_cast<int>(a.green() + (b.green() - a.green()) * t),
    static_cast<int>(a.blue()  + (b.blue()  - a.blue())  * t),
    static_cast<int>(a.alpha() + (b.alpha() - a.alpha()) * t)
  );
}

QColor HeatExchangerWidget::hotColor(double t) const {
  return lerpColor(kHotInlet, kHotOutlet, t);
}

QColor HeatExchangerWidget::coldColor(double t) const {
  return lerpColor(kColdInlet, kColdOutlet, t);
}

// ============================================================================
// Paint
// ============================================================================

void HeatExchangerWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::TextAntialiasing, true);

  // Recompute layout
  int ctrlH = 44;
  double availW = static_cast<double>(width());
  double availH = static_cast<double>(height() - ctrlH);
  margin_ = std::max(50.0, std::min(availW, availH) * 0.08);
  headerW_ = std::max(40.0, availW * 0.06);
  shellX_ = margin_ + headerW_;
  shellY_ = margin_ + 40;
  shellW_ = availW - 2 * margin_ - 2 * headerW_;
  shellH_ = availH - 2 * margin_ - 60;
  if (shellW_ < 100) shellW_ = 100;
  if (shellH_ < 60) shellH_ = 60;

  // Apply zoom
  if (std::abs(zoomLevel_ - 1.0) > 0.01) {
    double cx = width() / 2.0;
    double cy = (height() - ctrlH) / 2.0;
    p.translate(cx, cy);
    p.scale(zoomLevel_, zoomLevel_);
    p.translate(-cx, -cy);
  }

  drawBackground(p);
  drawSupports(p);
  drawShell(p);
  drawFoulingLayer(p);
  drawTubes(p);
  drawBaffles(p);
  drawHeaders(p);
  drawNozzles(p);
  drawParticles(p);
  drawFlowArrows(p);
  drawLabels(p);
  drawLegend(p);
  drawDataOverlay(p);
}

// ============================================================================
// Drawing methods
// ============================================================================

void HeatExchangerWidget::drawBackground(QPainter &p) {
  // Dark blueprint background
  p.fillRect(rect(), kBgDark);

  // Grid lines
  QPen gridPen(kGridLine, 0.5);
  p.setPen(gridPen);
  int step = 40;
  for (int x = 0; x < width(); x += step) {
    p.drawLine(x, 0, x, height());
  }
  for (int y = 0; y < height(); y += step) {
    p.drawLine(0, y, width(), y);
  }

  // Title
  QFont titleFont("Segoe UI", 14, QFont::Bold);
  p.setFont(titleFont);
  p.setPen(QColor(180, 210, 255));
  p.drawText(QRectF(0, 4, width(), 35), Qt::AlignCenter,
             "Shell & Tube Heat Exchanger - Digital Twin");
}

void HeatExchangerWidget::drawShell(QPainter &p) {
  // Outer shell body with metallic gradient
  QLinearGradient shellGrad(shellX_, shellY_, shellX_, shellY_ + shellH_);
  shellGrad.setColorAt(0.0, QColor(180, 188, 196));
  shellGrad.setColorAt(0.3, QColor(200, 206, 212));
  shellGrad.setColorAt(0.5, QColor(210, 215, 220));
  shellGrad.setColorAt(0.7, QColor(190, 196, 204));
  shellGrad.setColorAt(1.0, QColor(150, 158, 168));

  QPainterPath shellPath;
  double r = shellH_ * 0.15;
  shellPath.addRoundedRect(shellX_, shellY_, shellW_, shellH_, r, r);

  // Shell outline
  p.setPen(QPen(QColor(100, 110, 125), 3.0));
  p.setBrush(shellGrad);
  p.drawPath(shellPath);

  // Inner shell fill (cold fluid area - blue tint)
  double inset = 6.0;
  QLinearGradient innerGrad(shellX_, shellY_, shellX_, shellY_ + shellH_);
  innerGrad.setColorAt(0.0, QColor(40, 100, 200, 80));
  innerGrad.setColorAt(0.5, QColor(30, 80, 180, 60));
  innerGrad.setColorAt(1.0, QColor(50, 110, 210, 80));

  QPainterPath innerPath;
  innerPath.addRoundedRect(shellX_ + inset, shellY_ + inset,
                            shellW_ - 2 * inset, shellH_ - 2 * inset,
                            r - 2, r - 2);
  p.setPen(Qt::NoPen);
  p.setBrush(innerGrad);
  p.drawPath(innerPath);
}

void HeatExchangerWidget::drawTubes(QPainter &p) {
  double tubeSpacing = shellH_ / (visibleTubes_ + 1);
  double tubeLen = shellW_ - 12;
  double tubeStartX = shellX_ + 6;

  // Whether we have a proper axial T(x) profile from the finite-volume model.
  const bool haveAxial = hasState_ && !state_.Th_axial.empty();
  const double Tspan = std::max(1.0, op_.Tin_hot - op_.Tin_cold);

  for (int i = 0; i < visibleTubes_; ++i) {
    double cy = shellY_ + tubeSpacing * (i + 1);

    QLinearGradient tubeGrad(tubeStartX, cy, tubeStartX + tubeLen, cy);

    if (haveAxial) {
      // Paint an actual T(x) gradient derived from the axial cell array:
      // index 0 = hot inlet side (x=0), index N-1 = hot outlet side (x=L).
      const int N = static_cast<int>(state_.Th_axial.size());
      for (int k = 0; k < N; ++k) {
        const double x = (N == 1) ? 0.5 : static_cast<double>(k) / (N - 1);
        const double Tk = state_.Th_axial[static_cast<size_t>(k)];
        // Normalize into [0,1] with 1 = hot, 0 = cold. Clamp so disturbance
        // overshoots don't blow the color map out of range.
        double normT = (Tk - op_.Tin_cold) / Tspan;
        normT = std::max(0.0, std::min(1.0, normT));
        // hotColor(t) is parameterised so t=0 is hottest, t=1 is coolest.
        const QColor c = hotColor(1.0 - normT);
        tubeGrad.setColorAt(x, c);
      }
    } else {
      // Lumped / pre-simulation fallback: 2-stop gradient inlet → outlet.
      QColor cStart = hotColor(0.0);
      QColor cEnd = hotColor(1.0);
      if (hasState_) {
        double normT = (state_.Th_out - op_.Tin_cold) / Tspan;
        cEnd = lerpColor(kHotOutlet, QColor(255, 200, 100), 1.0 - normT);
      }
      tubeGrad.setColorAt(0.0, cStart);
      tubeGrad.setColorAt(1.0, cEnd);
    }

    // Draw tube with gradient
    double tubeThick = std::max(3.0, tubeSpacing * 0.28);
    p.setPen(Qt::NoPen);
    p.setBrush(tubeGrad);
    p.drawRoundedRect(QRectF(tubeStartX, cy - tubeThick / 2, tubeLen, tubeThick),
                       tubeThick / 2, tubeThick / 2);

    // Tube wall outline
    p.setPen(QPen(QColor(80, 50, 30, 120), 0.8));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(tubeStartX, cy - tubeThick / 2 - 1, tubeLen, tubeThick + 2),
                       tubeThick / 2 + 1, tubeThick / 2 + 1);
  }
}

void HeatExchangerWidget::drawFoulingLayer(QPainter &p) {
  if (foulingFraction_ < 0.01) return;

  double tubeSpacing = shellH_ / (visibleTubes_ + 1);
  double tubeLen = shellW_ - 12;
  double tubeStartX = shellX_ + 6;
  double maxThick = tubeSpacing * 0.18;
  double foulingThick = maxThick * foulingFraction_;

  QColor foulingCol = kFoulingColor;
  foulingCol.setAlphaF(0.3 + 0.5 * foulingFraction_);

  // tubeThick and the base alpha are loop-invariant — hoist them once so the
  // inner dx loop (hundreds of iterations per tube) avoids repeated max() and
  // QColor::alphaF() calls on every step.
  const double tubeThick = std::max(3.0, tubeSpacing * 0.28);
  const double halfThick = tubeThick * 0.5;
  const double baseAlpha = foulingCol.alphaF();

  p.setPen(Qt::NoPen);
  for (int i = 0; i < visibleTubes_; ++i) {
    const double cy = shellY_ + tubeSpacing * (i + 1);
    const double phase1 = i * 1.7;
    const double phase2 = static_cast<double>(i);

    // Draw fouling as a rough coating
    for (double dx = 0; dx < tubeLen; dx += 3.0) {
      const double localFoul = foulingThick * (0.7 + 0.3 * std::sin(dx * 0.15 + phase1));
      const double x = tubeStartX + dx;
      QColor c = foulingCol;
      c.setAlphaF(baseAlpha * (0.6 + 0.4 * std::sin(dx * 0.08 + phase2)));

      p.setBrush(c);
      // Top fouling
      p.drawRect(QRectF(x, cy - halfThick - localFoul, 3.0, localFoul));
      // Bottom fouling
      p.drawRect(QRectF(x, cy + halfThick, 3.0, localFoul));
    }
  }
}

void HeatExchangerWidget::drawBaffles(QPainter &p) {
  int nBaffles = hasGeometry_ ? std::max(3, geom_.nBaffles) : 8;
  double spacing = shellW_ / (nBaffles + 1);
  double baffleCut = hasGeometry_ ? geom_.baffleCutFrac : 0.25;

  for (int i = 0; i < nBaffles; ++i) {
    double bx = shellX_ + spacing * (i + 1);
    double cutH = shellH_ * baffleCut;

    // Alternating top/bottom cut
    double by, bh;
    if (i % 2 == 0) {
      by = shellY_;
      bh = shellH_ - cutH;
    } else {
      by = shellY_ + cutH;
      bh = shellH_ - cutH;
    }

    // Baffle plate
    QLinearGradient baffleGrad(bx - 2, by, bx + 2, by);
    baffleGrad.setColorAt(0.0, QColor(80, 130, 180));
    baffleGrad.setColorAt(0.5, QColor(110, 160, 210));
    baffleGrad.setColorAt(1.0, QColor(80, 130, 180));

    p.setPen(QPen(QColor(60, 100, 150), 0.5));
    p.setBrush(baffleGrad);
    p.drawRect(QRectF(bx - 2, by, 4, bh));
  }
}

void HeatExchangerWidget::drawHeaders(QPainter &p) {
  // Left header (tube sheet + bonnet)
  QLinearGradient leftGrad(margin_, shellY_, margin_ + headerW_, shellY_);
  leftGrad.setColorAt(0.0, QColor(160, 166, 174));
  leftGrad.setColorAt(0.5, QColor(185, 190, 198));
  leftGrad.setColorAt(1.0, QColor(170, 175, 182));

  QPainterPath leftHeader;
  // Rounded left cap
  leftHeader.moveTo(shellX_, shellY_);
  leftHeader.lineTo(margin_ + headerW_ * 0.4, shellY_);
  leftHeader.quadTo(margin_, shellY_, margin_, shellY_ + shellH_ * 0.5);
  leftHeader.quadTo(margin_, shellY_ + shellH_, margin_ + headerW_ * 0.4, shellY_ + shellH_);
  leftHeader.lineTo(shellX_, shellY_ + shellH_);
  leftHeader.closeSubpath();

  p.setPen(QPen(QColor(100, 110, 125), 2.5));
  p.setBrush(leftGrad);
  p.drawPath(leftHeader);

  // Tube sheet (vertical line at shell start)
  p.setPen(QPen(QColor(90, 95, 105), 3.0));
  p.drawLine(QPointF(shellX_, shellY_), QPointF(shellX_, shellY_ + shellH_));

  // Right header (return bonnet or fixed end)
  double rightX = shellX_ + shellW_;
  QLinearGradient rightGrad(rightX, shellY_, rightX + headerW_, shellY_);
  rightGrad.setColorAt(0.0, QColor(170, 175, 182));
  rightGrad.setColorAt(0.5, QColor(185, 190, 198));
  rightGrad.setColorAt(1.0, QColor(160, 166, 174));

  QPainterPath rightHeader;
  rightHeader.moveTo(rightX, shellY_);
  rightHeader.lineTo(rightX + headerW_ * 0.6, shellY_);
  rightHeader.quadTo(rightX + headerW_, shellY_, rightX + headerW_, shellY_ + shellH_ * 0.5);
  rightHeader.quadTo(rightX + headerW_, shellY_ + shellH_,
                      rightX + headerW_ * 0.6, shellY_ + shellH_);
  rightHeader.lineTo(rightX, shellY_ + shellH_);
  rightHeader.closeSubpath();

  p.setPen(QPen(QColor(100, 110, 125), 2.5));
  p.setBrush(rightGrad);
  p.drawPath(rightHeader);

  // Right tube sheet
  p.setPen(QPen(QColor(90, 95, 105), 3.0));
  p.drawLine(QPointF(rightX, shellY_), QPointF(rightX, shellY_ + shellH_));
}

void HeatExchangerWidget::drawNozzles(QPainter &p) {
  double nozzleW = 18;
  double nozzleH = shellH_ * 0.32;
  double flangeW = 26;
  double flangeH = 8;

  // --- Hot fluid inlet nozzle (top-left, into header) ---
  double hotInX = margin_ + headerW_ * 0.3;
  double hotInY = shellY_ - nozzleH;

  // Nozzle pipe
  QLinearGradient hotInGrad(hotInX, hotInY, hotInX + nozzleW, hotInY);
  hotInGrad.setColorAt(0.0, QColor(220, 80, 50));
  hotInGrad.setColorAt(0.5, QColor(245, 100, 65));
  hotInGrad.setColorAt(1.0, QColor(220, 80, 50));
  p.setPen(QPen(QColor(160, 50, 30), 1.5));
  p.setBrush(hotInGrad);
  p.drawRect(QRectF(hotInX, hotInY, nozzleW, nozzleH + 5));

  // Flange
  p.setBrush(kFlangeColor);
  p.setPen(QPen(QColor(120, 125, 130), 1.5));
  p.drawRect(QRectF(hotInX - (flangeW - nozzleW) / 2, hotInY - flangeH / 2, flangeW, flangeH));

  // --- Hot fluid outlet nozzle (bottom-right, from header) ---
  double hotOutX = shellX_ + shellW_ + headerW_ * 0.3;
  double hotOutY = shellY_ + shellH_;

  hotInGrad = QLinearGradient(hotOutX, hotOutY, hotOutX + nozzleW, hotOutY);
  hotInGrad.setColorAt(0.0, QColor(255, 150, 50));
  hotInGrad.setColorAt(0.5, QColor(255, 175, 80));
  hotInGrad.setColorAt(1.0, QColor(255, 150, 50));
  p.setPen(QPen(QColor(180, 100, 30), 1.5));
  p.setBrush(hotInGrad);
  p.drawRect(QRectF(hotOutX, hotOutY - 5, nozzleW, nozzleH));

  // Flange
  p.setBrush(kFlangeColor);
  p.setPen(QPen(QColor(120, 125, 130), 1.5));
  p.drawRect(QRectF(hotOutX - (flangeW - nozzleW) / 2,
                      hotOutY + nozzleH - flangeH / 2, flangeW, flangeH));

  // --- Cold fluid inlet nozzle (top-center, into shell) ---
  double coldInX = shellX_ + shellW_ * 0.35;
  double coldInY = shellY_ - nozzleH * 0.8;

  QLinearGradient coldInGrad(coldInX, coldInY, coldInX + nozzleW, coldInY);
  coldInGrad.setColorAt(0.0, QColor(40, 100, 220));
  coldInGrad.setColorAt(0.5, QColor(60, 130, 245));
  coldInGrad.setColorAt(1.0, QColor(40, 100, 220));
  p.setPen(QPen(QColor(25, 70, 160), 1.5));
  p.setBrush(coldInGrad);
  p.drawRect(QRectF(coldInX, coldInY, nozzleW, nozzleH * 0.8 + 5));

  // Flange
  p.setBrush(kFlangeColor);
  p.setPen(QPen(QColor(120, 125, 130), 1.5));
  p.drawRect(QRectF(coldInX - (flangeW - nozzleW) / 2, coldInY - flangeH / 2, flangeW, flangeH));

  // --- Cold fluid outlet nozzle (bottom, opposite side) ---
  double coldOutX = shellX_ + shellW_ * 0.65;
  double coldOutY = shellY_ + shellH_;

  coldInGrad = QLinearGradient(coldOutX, coldOutY, coldOutX + nozzleW, coldOutY);
  coldInGrad.setColorAt(0.0, QColor(80, 170, 245));
  coldInGrad.setColorAt(0.5, QColor(100, 200, 255));
  coldInGrad.setColorAt(1.0, QColor(80, 170, 245));
  p.setPen(QPen(QColor(50, 120, 190), 1.5));
  p.setBrush(coldInGrad);
  p.drawRect(QRectF(coldOutX, coldOutY - 5, nozzleW, nozzleH * 0.8));

  // Flange
  p.setBrush(kFlangeColor);
  p.setPen(QPen(QColor(120, 125, 130), 1.5));
  p.drawRect(QRectF(coldOutX - (flangeW - nozzleW) / 2,
                      coldOutY + nozzleH * 0.8 - flangeH / 2, flangeW, flangeH));
}

void HeatExchangerWidget::drawParticles(QPainter &p) {
  if (!simulationRunning_ && !hasState_) return;  // No particles when idle

  p.setPen(Qt::NoPen);

  for (const auto &pt : particles_) {
    if (pt.isTubeSide) {
      // Hot particles: bright flame-like appearance that contrasts with orange tubes.
      // Outer red/orange glow -> inner bright yellow -> white hot core.
      QColor outerCol = hotColor(pt.progress);
      double rCore = 2.5;
      double rMid  = 5.0;
      double rGlow = 9.0;

      // Outer red glow (subtle, extends halo)
      QRadialGradient outer(pt.x, pt.y, rGlow);
      outer.setColorAt(0.0, QColor(255, 80, 40, 170));
      outer.setColorAt(0.5, QColor(outerCol.red(), outerCol.green(), outerCol.blue(), 80));
      outer.setColorAt(1.0, QColor(outerCol.red(), outerCol.green(), outerCol.blue(), 0));
      p.setBrush(outer);
      p.drawEllipse(QPointF(pt.x, pt.y), rGlow, rGlow);

      // Middle yellow-orange layer
      QRadialGradient mid(pt.x, pt.y, rMid);
      mid.setColorAt(0.0, QColor(255, 240, 150, 255));
      mid.setColorAt(0.6, QColor(255, 180, 60, 230));
      mid.setColorAt(1.0, QColor(255, 100, 40, 0));
      p.setBrush(mid);
      p.drawEllipse(QPointF(pt.x, pt.y), rMid, rMid);

      // White-hot core — ensures visibility against any background
      p.setBrush(QColor(255, 255, 255, 255));
      p.drawEllipse(QPointF(pt.x, pt.y), rCore, rCore);
    } else {
      // Cold particles: ice-blue droplet with bright cyan core.
      QColor outerCol = coldColor(pt.progress);
      double rCore = 2.0;
      double rMid  = 4.2;
      double rGlow = 7.5;

      QRadialGradient outer(pt.x, pt.y, rGlow);
      outer.setColorAt(0.0, QColor(80, 180, 255, 150));
      outer.setColorAt(0.6, QColor(outerCol.red(), outerCol.green(), outerCol.blue(), 60));
      outer.setColorAt(1.0, QColor(outerCol.red(), outerCol.green(), outerCol.blue(), 0));
      p.setBrush(outer);
      p.drawEllipse(QPointF(pt.x, pt.y), rGlow, rGlow);

      QRadialGradient mid(pt.x, pt.y, rMid);
      mid.setColorAt(0.0, QColor(220, 240, 255, 255));
      mid.setColorAt(0.7, QColor(60, 160, 255, 220));
      mid.setColorAt(1.0, QColor(30, 120, 255, 0));
      p.setBrush(mid);
      p.drawEllipse(QPointF(pt.x, pt.y), rMid, rMid);

      // Bright white core
      p.setBrush(QColor(240, 250, 255, 255));
      p.drawEllipse(QPointF(pt.x, pt.y), rCore, rCore);
    }
  }
}

void HeatExchangerWidget::drawFlowArrows(QPainter &p) {
  auto drawArrow = [&](double x, double y, double angle, double size, const QColor &col) {
    p.save();
    p.translate(x, y);
    p.rotate(angle);
    QPainterPath arrow;
    arrow.moveTo(size, 0);
    arrow.lineTo(-size * 0.5, -size * 0.5);
    arrow.lineTo(-size * 0.3, 0);
    arrow.lineTo(-size * 0.5, size * 0.5);
    arrow.closeSubpath();
    p.setPen(QPen(col.darker(120), 1.0));
    p.setBrush(col);
    p.drawPath(arrow);
    p.restore();
  };

  // Hot flow direction arrows along tubes
  for (int i = 0; i < 3; ++i) {
    double ax = shellX_ + shellW_ * (0.25 + 0.25 * i);
    drawArrow(ax, shellY_ + shellH_ * 0.2, 0, 10, QColor(255, 120, 50, 180));
    drawArrow(ax, shellY_ + shellH_ * 0.8, 0, 10, QColor(255, 120, 50, 180));
  }

  // Cold flow arrows (downward in shell)
  double coldArrowX = shellX_ + shellW_ * 0.35 + 9;
  drawArrow(coldArrowX, shellY_ - 5, 90, 10, QColor(60, 150, 255, 200));

  double coldOutArrowX = shellX_ + shellW_ * 0.65 + 9;
  drawArrow(coldOutArrowX, shellY_ + shellH_ + 5, 90, 10, QColor(120, 200, 255, 200));

  // Hot inlet/outlet arrows
  double hotInArrowX = margin_ + headerW_ * 0.3 + 9;
  drawArrow(hotInArrowX, shellY_ - shellH_ * 0.2, 90, 10, QColor(240, 70, 40, 200));

  double hotOutArrowX = shellX_ + shellW_ + headerW_ * 0.3 + 9;
  drawArrow(hotOutArrowX, shellY_ + shellH_ + shellH_ * 0.2, 90, 10, QColor(255, 160, 70, 200));
}

void HeatExchangerWidget::drawLabels(QPainter &p) {
  QFont labelFont("Segoe UI", 8, QFont::Bold);
  p.setFont(labelFont);

  // QFontMetrics construction queries the font cache — hoist it out of the
  // lambda so the nine label calls below share a single instance per paint.
  const QFontMetrics fm(labelFont);

  auto drawLabel = [&](const QString &text, double x, double y,
                        const QColor &textCol = kLabelColor) {
    QRectF textRect = fm.boundingRect(text);
    double pad = 4;
    QRectF bgRect(x - textRect.width() / 2 - pad, y - textRect.height() / 2 - pad,
                   textRect.width() + 2 * pad, textRect.height() + 2 * pad);

    p.setPen(Qt::NoPen);
    p.setBrush(kLabelBg);
    p.drawRoundedRect(bgRect, 3, 3);

    p.setPen(textCol);
    p.drawText(bgRect, Qt::AlignCenter, text);
  };

  // Component labels
  drawLabel("Shell", shellX_ + shellW_ * 0.85, shellY_ - 15);
  drawLabel("Tubes", shellX_ + shellW_ * 0.5, shellY_ + shellH_ * 0.5);
  drawLabel("Header", margin_ + headerW_ * 0.5, shellY_ + shellH_ + 20);

  // Baffle label
  int nBaffles = hasGeometry_ ? std::max(3, geom_.nBaffles) : 8;
  double baffleX = shellX_ + shellW_ / (nBaffles + 1) * 2;
  drawLabel("Baffles", baffleX, shellY_ + shellH_ + 20);

  // Fluid labels
  QFont fluidFont("Segoe UI", 9, QFont::Bold);
  p.setFont(fluidFont);

  // Hot fluid label
  double hotInX = margin_ + headerW_ * 0.3 + 9;
  double hotInY = shellY_ - shellH_ * 0.32 - 20;
  drawLabel("Hot Fluid In", hotInX, hotInY, QColor(255, 140, 100));

  double hotOutX = shellX_ + shellW_ + headerW_ * 0.3 + 9;
  double hotOutY = shellY_ + shellH_ + shellH_ * 0.32 + 25;
  drawLabel("Hot Fluid Out", hotOutX, hotOutY, QColor(255, 190, 120));

  // Cold fluid label
  double coldInX = shellX_ + shellW_ * 0.35 + 9;
  double coldInY = shellY_ - shellH_ * 0.25 - 15;
  drawLabel("Cold Fluid In", coldInX, coldInY, QColor(100, 180, 255));

  double coldOutX = shellX_ + shellW_ * 0.65 + 9;
  double coldOutY = shellY_ + shellH_ + shellH_ * 0.25 + 20;
  drawLabel("Cold Fluid Out", coldOutX, coldOutY, QColor(160, 220, 255));
}

void HeatExchangerWidget::drawLegend(QPainter &p) {
  double lx = width() - 175;
  double ly = 45;
  double lw = 160;
  double lh = 115;

  // Background
  p.setPen(QPen(QColor(60, 100, 160), 1.5));
  p.setBrush(QColor(15, 30, 55, 230));
  p.drawRoundedRect(QRectF(lx, ly, lw, lh), 6, 6);

  QFont legendFont("Segoe UI", 8, QFont::Bold);
  p.setFont(legendFont);

  double ty = ly + 12;
  double spacing = 18;

  p.setPen(QColor(180, 200, 230));
  p.drawText(QRectF(lx + 8, ty, lw - 16, 16), Qt::AlignLeft, "Legend");
  ty += spacing + 2;

  // Hot fluid
  p.setBrush(kHotInlet);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPointF(lx + 16, ty + 4), 5, 5);
  p.setPen(QColor(255, 140, 100));
  p.drawText(QRectF(lx + 28, ty - 4, lw - 36, 16), Qt::AlignLeft, "Hot Fluid (Tube)");
  ty += spacing;

  // Cold fluid
  p.setBrush(kColdInlet);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPointF(lx + 16, ty + 4), 5, 5);
  p.setPen(QColor(100, 180, 255));
  p.drawText(QRectF(lx + 28, ty - 4, lw - 36, 16), Qt::AlignLeft, "Cold Fluid (Shell)");
  ty += spacing;

  // Fouling
  p.setBrush(kFoulingColor);
  p.setPen(Qt::NoPen);
  p.drawRect(QRectF(lx + 11, ty, 10, 8));
  p.setPen(QColor(200, 160, 100));
  p.drawText(QRectF(lx + 28, ty - 4, lw - 36, 16), Qt::AlignLeft, "Fouling Deposit");
  ty += spacing;

  // Baffles
  p.setBrush(kBaffleColor);
  p.setPen(Qt::NoPen);
  p.drawRect(QRectF(lx + 13, ty, 6, 10));
  p.setPen(QColor(120, 170, 220));
  p.drawText(QRectF(lx + 28, ty - 2, lw - 36, 16), Qt::AlignLeft, "Baffles");
}

void HeatExchangerWidget::drawDataOverlay(QPainter &p) {
  if (!hasState_) return;

  double ox = 12;
  double oy = 45;
  double ow = 155;
  double oh = 155;

  p.setPen(QPen(QColor(60, 100, 160), 1.5));
  p.setBrush(QColor(15, 30, 55, 230));
  p.drawRoundedRect(QRectF(ox, oy, ow, oh), 6, 6);

  QFont dataFont("Consolas", 8);
  p.setFont(dataFont);

  double ty = oy + 10;
  double spacing = 16;

  auto drawRow = [&](const QString &label, const QString &value, const QColor &col) {
    p.setPen(QColor(140, 160, 190));
    p.drawText(QRectF(ox + 8, ty, 80, 14), Qt::AlignLeft, label);
    p.setPen(col);
    p.drawText(QRectF(ox + 80, ty, 70, 14), Qt::AlignRight, value);
    ty += spacing;
  };

  p.setPen(QColor(180, 200, 230));
  QFont titleFont("Segoe UI", 8, QFont::Bold);
  p.setFont(titleFont);
  p.drawText(QRectF(ox + 8, ty, ow - 16, 14), Qt::AlignLeft, "Live Data");
  ty += spacing + 2;
  p.setFont(dataFont);

  drawRow("Th out:", QString("%1 C").arg(state_.Th_out, 0, 'f', 1), QColor(255, 140, 100));
  drawRow("Tc out:", QString("%1 C").arg(state_.Tc_out, 0, 'f', 1), QColor(100, 180, 255));
  drawRow("Q:", QString("%1 kW").arg(state_.Q / 1000.0, 0, 'f', 1), QColor(255, 220, 100));
  drawRow("U:", QString("%1 W/m2K").arg(state_.U, 0, 'f', 0), QColor(180, 230, 180));
  drawRow("Rf:", QString("%1 m\xC2\xB2\xC2\xB7K/W").arg(state_.Rf, 0, 'e', 2),
           QColor(200, 160, 100));
  drawRow("dP tube:", QString("%1 Pa").arg(state_.dP_tube, 0, 'f', 0), QColor(255, 160, 160));
  drawRow("dP shell:", QString("%1 Pa").arg(state_.dP_shell, 0, 'f', 0), QColor(160, 200, 255));

  // Fouling bar
  ty += 4;
  p.setPen(QColor(140, 160, 190));
  p.drawText(QRectF(ox + 8, ty, 60, 12), Qt::AlignLeft, "Fouling:");

  double barX = ox + 62;
  double barW = ow - 74;
  double barH = 8;
  p.setPen(QPen(QColor(80, 100, 130), 1));
  p.setBrush(QColor(30, 50, 80));
  p.drawRoundedRect(QRectF(barX, ty + 2, barW, barH), 3, 3);

  QLinearGradient barGrad(barX, ty, barX + barW * foulingFraction_, ty);
  barGrad.setColorAt(0.0, QColor(100, 180, 80));
  barGrad.setColorAt(0.5, QColor(220, 180, 50));
  barGrad.setColorAt(1.0, QColor(220, 60, 40));

  p.setPen(Qt::NoPen);
  p.setBrush(barGrad);
  p.drawRoundedRect(QRectF(barX + 1, ty + 3, (barW - 2) * foulingFraction_, barH - 2), 2, 2);
}

void HeatExchangerWidget::drawSupports(QPainter &p) {
  double supportW = 20;
  double supportH = 25;
  double baseY = shellY_ + shellH_;

  // Two support legs
  double x1 = shellX_ + shellW_ * 0.2;
  double x2 = shellX_ + shellW_ * 0.8;

  QLinearGradient supportGrad(0, baseY, 0, baseY + supportH);
  supportGrad.setColorAt(0.0, QColor(140, 145, 150));
  supportGrad.setColorAt(1.0, QColor(100, 105, 110));

  p.setPen(QPen(QColor(80, 85, 90), 1.5));
  p.setBrush(supportGrad);

  // Left support
  QPainterPath s1;
  s1.moveTo(x1 - supportW, baseY + supportH);
  s1.lineTo(x1 - supportW * 0.3, baseY);
  s1.lineTo(x1 + supportW * 0.3, baseY);
  s1.lineTo(x1 + supportW, baseY + supportH);
  s1.closeSubpath();
  p.drawPath(s1);

  // Right support
  QPainterPath s2;
  s2.moveTo(x2 - supportW, baseY + supportH);
  s2.lineTo(x2 - supportW * 0.3, baseY);
  s2.lineTo(x2 + supportW * 0.3, baseY);
  s2.lineTo(x2 + supportW, baseY + supportH);
  s2.closeSubpath();
  p.drawPath(s2);

  // Base plates
  p.setBrush(QColor(100, 105, 110));
  p.drawRect(QRectF(x1 - supportW - 5, baseY + supportH, supportW * 2 + 10, 4));
  p.drawRect(QRectF(x2 - supportW - 5, baseY + supportH, supportW * 2 + 10, 4));
}
