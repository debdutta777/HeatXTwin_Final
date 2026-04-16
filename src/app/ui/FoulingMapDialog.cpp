#include "FoulingMapDialog.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QFontMetrics>
#include <QLocale>
#include <QToolTip>
#include <QMouseEvent>

#include <algorithm>
#include <cmath>

namespace {

QString fmtRf(double v) {
  return QStringLiteral("%1 \xC3\x97 10\xE2\x81\xBB\xE2\x81\xB4 m\xC2\xB2\xC2\xB7K/W")
          .arg(QLocale::c().toString(v * 1.0e4, 'f', 3));
}

/** Viridis-inspired 5-stop palette: deep indigo → teal → yellow → red. */
QColor viridisColor(double t) {
  t = std::clamp(t, 0.0, 1.0);
  struct Stop { double t; QColor c; };
  static const Stop stops[] = {
      {0.00, QColor( 68,   1,  84)},   // deep indigo
      {0.25, QColor( 59,  82, 139)},   // blue
      {0.50, QColor( 33, 145, 140)},   // teal
      {0.75, QColor(178, 221,  53)},   // yellow-green
      {1.00, QColor(253, 231,  37)},   // bright yellow (low-fouling end)
  };
  // Flip so LOW Rf = green and HIGH Rf = red.  (Viridis naturally goes dark
  // → bright; for a fouling "danger" plot we want RED at the high end.)
  static const Stop hotStops[] = {
      {0.00, QColor( 46, 134,  71)},   // green (clean)
      {0.25, QColor(196, 214,  68)},   // yellow-green
      {0.50, QColor(245, 215,  30)},   // yellow
      {0.75, QColor(230, 130,  30)},   // orange
      {1.00, QColor(186,  32,  28)},   // deep red (hot spot)
  };
  (void)stops;
  for (size_t i = 1; i < sizeof(hotStops) / sizeof(hotStops[0]); ++i) {
    if (t <= hotStops[i].t) {
      const double span = hotStops[i].t - hotStops[i - 1].t;
      const double f    = (span > 1e-9) ? (t - hotStops[i - 1].t) / span : 0.0;
      auto mix = [f](int a, int b) { return static_cast<int>(a + (b - a) * f); };
      return QColor(mix(hotStops[i - 1].c.red(),   hotStops[i].c.red()),
                    mix(hotStops[i - 1].c.green(), hotStops[i].c.green()),
                    mix(hotStops[i - 1].c.blue(),  hotStops[i].c.blue()));
    }
  }
  return hotStops[sizeof(hotStops) / sizeof(hotStops[0]) - 1].c;
}

/**
 *  Plan-view painter for the tube bundle.  Each tube is a filled circle whose
 *  colour encodes its axial-averaged Rf.  The shell inner diameter is drawn
 *  as an outline so the viewer can see which tubes are in the "window".
 */
class BundleView : public QWidget {
public:
  BundleView(const hx::FoulingMap &map, const hx::Geometry &g, QWidget *parent = nullptr)
      : QWidget(parent), map_(map), g_(g) {
    setMinimumSize(420, 420);
    setMouseTracking(true);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f6f9fc"));

    const double Rshell = 0.5 * g_.shellID;
    if (Rshell <= 0.0 || map_.tubes.empty()) {
      p.setPen(QColor("#7f8c8d"));
      p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No tubes to display."));
      return;
    }

    const int pad  = 18;
    const int side = std::min(width(), height()) - 2 * pad;
    const double scale = side / (2.0 * Rshell);
    const double cx = width()  * 0.5;
    const double cy = height() * 0.5;

    // Shell outline
    p.setPen(QPen(QColor("#34495e"), 1.6));
    p.setBrush(QColor(255, 255, 255));
    p.drawEllipse(QPointF(cx, cy), Rshell * scale, Rshell * scale);

    // Baffle-cut indicators (top and bottom of the circle).
    const double bc = g_.baffleCutFrac;
    const double cutY = Rshell * (1.0 - 2.0 * bc);  // y-offset to cut line
    p.setPen(QPen(QColor("#c0c9d1"), 1.0, Qt::DashLine));
    p.drawLine(QPointF(cx - Rshell * scale, cy - cutY * scale),
               QPointF(cx + Rshell * scale, cy - cutY * scale));
    p.drawLine(QPointF(cx - Rshell * scale, cy + cutY * scale),
               QPointF(cx + Rshell * scale, cy + cutY * scale));

    // Tube circles — colour-coded
    const double rDraw = std::max(2.0, 0.5 * g_.Do * scale);
    const double mn = map_.Rf_min;
    const double mx = std::max(map_.Rf_max, mn + 1e-12);
    for (size_t i = 0; i < map_.tubes.size(); ++i) {
      const hx::TubePos &t = map_.tubes[i];
      const double x = cx + t.x * scale;
      const double y = cy + t.y * scale;
      const double tN = (map_.Rf_mean[i] - mn) / (mx - mn);
      p.setPen(QPen(QColor(60, 60, 60, 140), 0.6));
      p.setBrush(viridisColor(tN));
      p.drawEllipse(QPointF(x, y), rDraw, rDraw);
    }

    // Title
    p.setPen(QColor("#2c3e50"));
    QFont f = p.font(); f.setBold(true); f.setPointSizeF(f.pointSizeF() + 0.5);
    p.setFont(f);
    p.drawText(QRectF(0, 2, width(), 18), Qt::AlignCenter,
               QStringLiteral("Bundle plan-view (axial-mean fouling)"));
  }

  void mouseMoveEvent(QMouseEvent *ev) override {
    const double Rshell = 0.5 * g_.shellID;
    if (Rshell <= 0.0 || map_.tubes.empty()) return;
    const int pad = 18;
    const int side = std::min(width(), height()) - 2 * pad;
    const double scale = side / (2.0 * Rshell);
    const double cx = width()  * 0.5;
    const double cy = height() * 0.5;
    const double rHit = std::max(2.0, 0.5 * g_.Do * scale);

    for (size_t i = 0; i < map_.tubes.size(); ++i) {
      const double x = cx + map_.tubes[i].x * scale;
      const double y = cy + map_.tubes[i].y * scale;
      const QPointF posF = ev->position();
      if (std::hypot(posF.x() - x, posF.y() - y) < rHit) {
        QToolTip::showText(ev->globalPosition().toPoint(),
            QStringLiteral("Tube #%1\nr/R = %2\nRf_mean = %3")
                .arg(map_.tubes[i].id)
                .arg(map_.tubes[i].r_norm, 0, 'f', 3)
                .arg(fmtRf(map_.Rf_mean[i])), this);
        return;
      }
    }
    QToolTip::hideText();
  }

private:
  const hx::FoulingMap &map_;
  const hx::Geometry   &g_;
};

/**
 *  Axial strip-chart: Rf(ζ) averaged across the bundle.  Rendered as a
 *  horizontal coloured bar so a quick glance shows "fouling grows from inlet
 *  to outlet" — the γ·z/L effect built into the model.
 */
class AxialStrip : public QWidget {
public:
  explicit AxialStrip(const hx::FoulingMap &map, QWidget *parent = nullptr)
      : QWidget(parent), map_(map) {
    setMinimumHeight(64);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f6f9fc"));

    const int padL = 44, padR = 18, padT = 18, padB = 22;
    const QRectF strip(padL, padT, width() - padL - padR, height() - padT - padB);
    if (strip.width() <= 0.0 || strip.height() <= 0.0 || map_.Rf_axial.empty()) return;

    const double mn = *std::min_element(map_.Rf_axial.begin(), map_.Rf_axial.end());
    const double mx = *std::max_element(map_.Rf_axial.begin(), map_.Rf_axial.end());
    const double span = std::max(mx - mn, 1e-12);

    const int N = static_cast<int>(map_.Rf_axial.size());
    const double cellW = strip.width() / N;
    for (int j = 0; j < N; ++j) {
      const double t = (map_.Rf_axial[static_cast<size_t>(j)] - mn) / span;
      p.fillRect(QRectF(strip.left() + j * cellW, strip.top(),
                        cellW + 0.5, strip.height()), viridisColor(t));
    }
    p.setPen(QColor("#34495e"));
    p.drawRect(strip);

    // Axis labels
    p.setPen(QColor("#2c3e50"));
    p.drawText(QRectF(0, padT - 14, width(), 14), Qt::AlignCenter,
               QStringLiteral("Axial-mean fouling  Rf(\xCE\xB6)  —  inlet \xE2\x86\x92 outlet"));
    p.drawText(QRectF(2, strip.top() - 2, padL - 4, strip.height()),
               Qt::AlignRight | Qt::AlignTop,   fmtRf(mx));
    p.drawText(QRectF(2, strip.bottom() - 14, padL - 4, 14),
               Qt::AlignRight | Qt::AlignBottom, fmtRf(mn));
    p.drawText(QRectF(padL, strip.bottom() + 2, strip.width(), 14),
               Qt::AlignLeft,  QStringLiteral("\xCE\xB6 = 0"));
    p.drawText(QRectF(padL, strip.bottom() + 2, strip.width(), 14),
               Qt::AlignRight, QStringLiteral("\xCE\xB6 = 1"));
  }

private:
  const hx::FoulingMap &map_;
};

/** Vertical colour-bar with min/mid/max tick labels, reading the current
 *  Rf_min / Rf_max from the shared FoulingMap reference so live updates are
 *  reflected automatically on the next repaint. */
class ColorBar : public QWidget {
public:
  explicit ColorBar(const hx::FoulingMap &map, QWidget *parent = nullptr)
      : QWidget(parent), map_(map) {
    setFixedWidth(74);
    setMinimumHeight(320);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f6f9fc"));

    const int padT = 20, padB = 20;
    const QRectF bar(10, padT, 22, height() - padT - padB);

    for (int i = 0; i < 100; ++i) {
      const double t = 1.0 - i / 99.0;  // top = high = red
      p.fillRect(QRectF(bar.left(), bar.top() + (bar.height() / 100.0) * i,
                        bar.width(), (bar.height() / 100.0) + 0.5),
                 viridisColor(t));
    }
    p.setPen(QColor("#34495e"));
    p.drawRect(bar);

    p.setPen(QColor("#2c3e50"));
    auto drawLabel = [&](double yy, double v) {
      const QRectF r(bar.right() + 4, yy - 8, width() - bar.right() - 6, 16);
      p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, fmtRf(v));
    };
    drawLabel(bar.top(),                     map_.Rf_max);
    drawLabel(bar.top() + bar.height() / 2,  0.5 * (map_.Rf_min + map_.Rf_max));
    drawLabel(bar.bottom(),                  map_.Rf_min);

    p.drawText(QRectF(0, 0, width(), padT), Qt::AlignCenter,
               QStringLiteral("R_f"));
  }

private:
  const hx::FoulingMap &map_;
};

} // anonymous namespace

FoulingMapDialog::FoulingMapDialog(const hx::FoulingMap &map,
                                     const hx::Geometry  &geom,
                                     QWidget *parent)
    : QDialog(parent), map_(map), geom_(geom) {
  setWindowTitle(QStringLiteral("Per-tube Fouling Heatmap"));
  resize(980, 680);

  auto *main = new QVBoxLayout(this);

  // Header — text is filled in by refreshSummaries().
  header_ = new QLabel(this);
  header_->setStyleSheet("QLabel{background:#eaf4fb;padding:10px 14px;"
                         "border:1px solid #b5d4ea;border-radius:4px;}");
  main->addWidget(header_);

  // Middle row: bundle plan-view + colour bar.  Child widgets keep
  // references to map_ / geom_ which live as long as this dialog, so
  // updateMap() can swap the contents in place and a simple update() call
  // is enough to repaint with the new data.
  auto *midRow = new QHBoxLayout();
  midRow->setSpacing(8);
  auto *bundle = new BundleView(map_, geom_, this);
  auto *bar    = new ColorBar(map_, this);
  bundleView_  = bundle;
  colorBar_    = bar;
  midRow->addWidget(bundle, 1);
  midRow->addWidget(bar);
  main->addLayout(midRow, 1);

  // Axial strip
  auto *strip = new AxialStrip(map_, this);
  axialStrip_ = strip;
  main->addWidget(strip);

  // Summary card — values filled in by refreshSummaries().
  auto *grid = new QGridLayout();
  grid->setSpacing(6);
  auto makeKey = [](const QString &t) {
    auto *l = new QLabel(t);
    l->setStyleSheet("color:#2c3e50;font-weight:600;");
    return l;
  };
  auto makeVal = []() {
    auto *l = new QLabel();
    l->setStyleSheet("color:#34495e;font-family:'Cambria Math',serif;");
    return l;
  };
  lblMeanOverall_ = makeVal();
  lblMin_         = makeVal();
  lblMax_         = makeVal();
  lblRatio_       = makeVal();
  lblHotSpot_     = makeVal();
  grid->addWidget(makeKey(QStringLiteral("Area-weighted mean Rf")), 0, 0);
  grid->addWidget(lblMeanOverall_,                                  0, 1);
  grid->addWidget(makeKey(QStringLiteral("Minimum tube Rf")),       1, 0);
  grid->addWidget(lblMin_,                                          1, 1);
  grid->addWidget(makeKey(QStringLiteral("Maximum tube Rf")),       2, 0);
  grid->addWidget(lblMax_,                                          2, 1);
  grid->addWidget(makeKey(QStringLiteral("Max / mean ratio")),      3, 0);
  grid->addWidget(lblRatio_,                                        3, 1);
  grid->addWidget(makeKey(QStringLiteral("Hot-spot fraction")),     4, 0);
  grid->addWidget(lblHotSpot_,                                      4, 1);

  auto *box = new QFrame(this);
  box->setFrameShape(QFrame::StyledPanel);
  box->setStyleSheet("QFrame{background:#fdfefe;border:1px solid #d5dae0;border-radius:4px;}");
  box->setLayout(grid);
  grid->setContentsMargins(14, 10, 14, 10);
  main->addWidget(box);

  // Physics legend
  auto *legend = new QLabel(this);
  legend->setText(QStringLiteral(
      "<div style='font-family:\"Cambria Math\",serif;color:#2c3e50;font-size:10pt;'>"
      "R<sub>f,local</sub>(r, y, \xCE\xB6) &nbsp;=&nbsp; "
      "R<sub>f,bulk</sub> \xC2\xB7 "
      "(1 + 0.8\xC2\xB7(1\xE2\x88\x92r\xC2\xB2)) \xC2\xB7 "
      "(1 + 0.25\xC2\xB7\xCF\x83(10(|y|/R \xE2\x88\x92" " (1\xE2\x88\x92" "bc)))) \xC2\xB7 "
      "(1 + 0.6\xC2\xB7\xCE\xB6) \xC2\xB7 "
      "\xE2\x88\x9A(2 / m\xCC\x87<sub>shell</sub>)<br>"
      "<i style='color:#566573;'>"
      "Radial bypass bias \xC2\xB7 Baffle-cut stagnation (\xCF\x83 = logistic, bc = cut fraction) \xC2\xB7 "
      "Axial build-up \xC2\xB7 Shear shield. "
      "Renormalised so area-averaged mean = R<sub>f,bulk</sub>."
      "</i></div>"));
  legend->setWordWrap(true);
  legend->setStyleSheet("QLabel{background:#f6f9fc;padding:8px 12px;"
                         "border:1px solid #d5dae0;border-radius:4px;}");
  main->addWidget(legend);

  // Close
  auto *btnRow = new QHBoxLayout();
  btnRow->addStretch();
  auto *close = new QPushButton(QStringLiteral("Close"), this);
  btnRow->addWidget(close);
  main->addLayout(btnRow);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);

  refreshSummaries();
}

void FoulingMapDialog::refreshSummaries() {
  const double mean  = map_.Rf_mean_overall;
  const double mn    = map_.Rf_min;
  const double mx    = map_.Rf_max;
  const double ratio = (mean > 1e-15) ? (mx / mean) : 0.0;

  if (header_) {
    header_->setText(QStringLiteral(
        "<div style='font-family:\"Cambria Math\",serif;color:#1f3b57;font-size:11pt;'>"
        "<b>Per-tube fouling distribution</b> &nbsp; \xE2\x80\x94 &nbsp; "
        "%1 tubes \xC3\x97 %2 axial cells &nbsp; \xE2\x80\xA2 &nbsp; "
        "bulk R<sub>f</sub> = %3"
        "</div>")
          .arg(static_cast<int>(map_.tubes.size()))
          .arg(map_.nAxial)
          .arg(fmtRf(mean)));
  }
  if (lblMeanOverall_) lblMeanOverall_->setText(fmtRf(mean));
  if (lblMin_)         lblMin_->setText(fmtRf(mn));
  if (lblMax_)         lblMax_->setText(fmtRf(mx));
  if (lblRatio_)       lblRatio_->setText(QStringLiteral("%1 \xC3\x97")
                                            .arg(QLocale::c().toString(ratio, 'f', 3)));
  if (lblHotSpot_)     lblHotSpot_->setText(QStringLiteral("%1 %")
                                              .arg(QLocale::c().toString(
                                                  map_.hot_spot_fraction * 100.0, 'f', 1)));
}

void FoulingMapDialog::updateMap(const hx::FoulingMap &map, const hx::Geometry &geom) {
  map_  = map;
  geom_ = geom;
  refreshSummaries();
  if (bundleView_) bundleView_->update();
  if (axialStrip_) axialStrip_->update();
  if (colorBar_)   colorBar_->update();
}
