#include "VibrationDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QTextBrowser>
#include <QLocale>
#include <QPainter>

#include <cmath>

namespace {

QString fmt(double v, int prec = 4) {
  if (!std::isfinite(v)) return QStringLiteral("—");
  return QLocale::c().toString(v, 'g', prec);
}

QColor statusColor(hx::VibrationResult::Status s) {
  switch (s) {
    case hx::VibrationResult::Status::Safe:     return QColor("#27ae60");  // green
    case hx::VibrationResult::Status::Marginal: return QColor("#f39c12");  // amber
    case hx::VibrationResult::Status::Fail:     return QColor("#c0392b");  // red
  }
  return QColor("#7f8c8d");
}

const char *statusText(hx::VibrationResult::Status s) {
  switch (s) {
    case hx::VibrationResult::Status::Safe:     return "SAFE";
    case hx::VibrationResult::Status::Marginal: return "MARGINAL";
    case hx::VibrationResult::Status::Fail:     return "FAIL";
  }
  return "UNKNOWN";
}

/**
 * Circular LED with a caption — used for the three status lights.
 */
class StatusLight : public QFrame {
public:
  StatusLight(const QString &title,
              const QString &criterion,
              hx::VibrationResult::Status status,
              double value,
              const QString &formula,
              QWidget *parent = nullptr)
      : QFrame(parent) {
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(QStringLiteral("QFrame{background:#fdfefe;border:1px solid #d5dae0;border-radius:4px;}"));

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(12, 10, 12, 10);
    lay->setSpacing(4);

    auto *lblTitle = new QLabel(title, this);
    lblTitle->setStyleSheet("font-weight:600;color:#2c3e50;font-size:11pt;");
    lay->addWidget(lblTitle);

    auto *row = new QHBoxLayout();
    row->setSpacing(10);

    auto *led = new QLabel(this);
    led->setFixedSize(42, 42);
    QPixmap pm(42, 42);
    pm.fill(Qt::transparent);
    {
      QPainter p(&pm);
      p.setRenderHint(QPainter::Antialiasing);
      p.setBrush(statusColor(status));
      p.setPen(QPen(statusColor(status).darker(140), 2));
      p.drawEllipse(3, 3, 36, 36);
    }
    led->setPixmap(pm);
    row->addWidget(led);

    auto *col = new QVBoxLayout();
    col->setSpacing(1);

    auto *lblStatus = new QLabel(statusText(status), this);
    lblStatus->setStyleSheet(QStringLiteral(
        "font-weight:700;color:%1;font-size:13pt;letter-spacing:0.05em;")
            .arg(statusColor(status).name()));
    col->addWidget(lblStatus);

    auto *lblValue = new QLabel(QStringLiteral("= %1").arg(fmt(value, 4)), this);
    lblValue->setStyleSheet("font-family:'Cambria Math',serif;font-size:12pt;color:#2c3e50;");
    col->addWidget(lblValue);

    row->addLayout(col);
    row->addStretch();
    lay->addLayout(row);

    auto *lblCriterion = new QLabel(criterion, this);
    lblCriterion->setStyleSheet("color:#566573;font-size:9pt;");
    lblCriterion->setWordWrap(true);
    lay->addWidget(lblCriterion);

    auto *lblFormula = new QLabel(formula, this);
    lblFormula->setStyleSheet(
        "font-family:'Cambria Math','Latin Modern Math',serif;"
        "font-style:italic;color:#34495e;font-size:9.5pt;"
        "background:#f6f9fc;padding:3px 6px;border-radius:3px;");
    lblFormula->setWordWrap(true);
    lay->addWidget(lblFormula);
  }
};

} // namespace

VibrationDialog::VibrationDialog(const hx::VibrationResult &r,
                                   const hx::VibrationConfig &cfg,
                                   QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("TEMA Flow-Induced Vibration Check"));
  resize(920, 720);

  auto *main = new QVBoxLayout(this);

  // --- Overall verdict banner ---------------------------------------------
  {
    auto *banner = new QLabel(this);
    banner->setAlignment(Qt::AlignCenter);
    banner->setText(QStringLiteral(
        "<div style='font-size:16pt;font-weight:700;color:white;'>"
        "Overall verdict: %1"
        "</div>"
        "<div style='color:white;font-size:10pt;margin-top:4px;'>%2</div>")
            .arg(statusText(r.overall))
            .arg(QString::fromStdString(r.summary)));
    banner->setWordWrap(true);
    banner->setStyleSheet(QStringLiteral(
        "QLabel{background:%1;padding:14px 18px;border-radius:4px;}")
            .arg(statusColor(r.overall).name()));
    main->addWidget(banner);
  }

  // --- Three status cards -------------------------------------------------
  auto *lightsRow = new QHBoxLayout();
  lightsRow->setSpacing(10);

  lightsRow->addWidget(new StatusLight(
      QStringLiteral("Vortex shedding"),
      QStringLiteral("Safe if fv / fn < 0.5 (lock-in risk at ~0.8)."),
      r.vortex,
      r.freq_ratio,
      QStringLiteral("fv / fn = (St · V / Do) / fn"),
      this));

  lightsRow->addWidget(new StatusLight(
      QStringLiteral("Fluid-elastic instability"),
      QStringLiteral("Safe if V_cross / V_crit < 1. Below 0.5 is the industry design margin."),
      r.fluidElastic,
      r.V_ratio,
      QStringLiteral("V_crit = K · fn · Do · √(m_e δ / (ρ Do²))   (Connors 1970, K=3.3)"),
      this));

  lightsRow->addWidget(new StatusLight(
      QStringLiteral("Acoustic resonance"),
      QStringLiteral("Safe if |f_a − fv| / f_a > 20%.  Mostly relevant for gas-in-shell."),
      r.acoustic,
      r.acoustic_margin,
      QStringLiteral("f_a ≈ c_sound / (2 Ds)      (1st transverse shell mode)"),
      this));

  main->addLayout(lightsRow);

  // --- Numerical details table -------------------------------------------
  auto *grid = new QGridLayout();
  grid->setSpacing(6);
  grid->setColumnStretch(1, 1);
  grid->setColumnStretch(3, 1);

  auto addRow = [grid](int r_, const QString &lbl1, const QString &val1,
                                 const QString &lbl2, const QString &val2) {
    auto make = [](const QString &s, bool emph) {
      auto *l = new QLabel(s);
      if (emph) l->setStyleSheet("color:#2c3e50;font-weight:600;");
      else      l->setStyleSheet("color:#34495e;font-family:'Cambria Math',serif;");
      return l;
    };
    grid->addWidget(make(lbl1, true),  r_, 0);
    grid->addWidget(make(val1, false), r_, 1);
    grid->addWidget(make(lbl2, true),  r_, 2);
    grid->addWidget(make(val2, false), r_, 3);
  };

  int rix = 0;
  addRow(rix++, QStringLiteral("Natural frequency fn"),   QStringLiteral("%1 Hz").arg(fmt(r.fn, 5)),
                  QStringLiteral("Unsupported span L"),    QStringLiteral("%1 m").arg(fmt(r.L_eff, 4)));
  addRow(rix++, QStringLiteral("Effective mass/length m_e"), QStringLiteral("%1 kg/m").arg(fmt(r.m_eff, 5)),
                  QStringLiteral("Moment of inertia I"),   QStringLiteral("%1 m⁴").arg(fmt(r.I_area, 4)));
  addRow(rix++, QStringLiteral("Flexural rigidity EI"),    QStringLiteral("%1 N·m²").arg(fmt(r.EI, 4)),
                  QStringLiteral("End-condition C"),        QStringLiteral("%1 (%2)").arg(fmt(r.C_end, 4))
                      .arg(cfg.end_support == 0 ? "pinned-pinned"
                            : cfg.end_support == 2 ? "clamped-pinned" : "clamped-clamped"));
  addRow(rix++, QStringLiteral("Crossflow area Sm (Kern)"), QStringLiteral("%1 m²").arg(fmt(r.Sm, 4)),
                  QStringLiteral("Crossflow velocity V"),   QStringLiteral("%1 m/s").arg(fmt(r.V_cross, 4)));
  addRow(rix++, QStringLiteral("Vortex-shedding fv = St·V/Do"), QStringLiteral("%1 Hz").arg(fmt(r.fv, 5)),
                  QStringLiteral("Strouhal number St"),     QStringLiteral("%1").arg(fmt(cfg.strouhal, 3)));
  addRow(rix++, QStringLiteral("Critical velocity V_crit"), QStringLiteral("%1 m/s").arg(fmt(r.V_crit, 5)),
                  QStringLiteral("Connors K · log-dec δ"),  QStringLiteral("%1 · %2")
                      .arg(fmt(cfg.connors_K, 3), fmt(cfg.damping_log_decrement, 3)));
  addRow(rix++, QStringLiteral("Speed of sound in shell"), QStringLiteral("%1 m/s").arg(fmt(r.c_sound, 4)),
                  QStringLiteral("Acoustic mode f_a"),      QStringLiteral("%1 Hz").arg(fmt(r.f_acoustic, 5)));

  auto *gridBox = new QFrame(this);
  gridBox->setFrameShape(QFrame::StyledPanel);
  gridBox->setStyleSheet("QFrame{background:#fdfefe;border:1px solid #d5dae0;border-radius:4px;}");
  gridBox->setLayout(grid);
  grid->setContentsMargins(14, 12, 14, 12);
  main->addWidget(gridBox);

  // --- Formula block -----------------------------------------------------
  auto *txt = new QTextBrowser(this);
  txt->setReadOnly(true);
  txt->setHtml(QStringLiteral(
      "<h3 style='margin-top:0;color:#2c3e50;'>References &amp; formulae</h3>"
      "<table cellpadding='4' style='font-family:\"Cambria Math\",serif;font-size:10.5pt;color:#2c3e50;'>"
      "<tr><td><b>Beam bending (Euler–Bernoulli)</b></td>"
      "<td>f<sub>n</sub> = (C / (2π L²)) √(EI / m<sub>e</sub>)</td></tr>"
      "<tr><td><b>Vortex shedding</b></td>"
      "<td>f<sub>v</sub> = St · V<sub>cross</sub> / D<sub>o</sub> &nbsp;&nbsp; (St ≈ 0.2 for triangular banks)</td></tr>"
      "<tr><td><b>Fluid-elastic (Connors 1970)</b></td>"
      "<td>V<sub>crit</sub> = K · f<sub>n</sub> · D<sub>o</sub> · √(m<sub>e</sub> δ / (ρ<sub>s</sub> D<sub>o</sub>²))</td></tr>"
      "<tr><td><b>Shell acoustic mode</b></td>"
      "<td>f<sub>a</sub> = c<sub>sound</sub> / (2 D<sub>s</sub>)</td></tr>"
      "</table>"
      "<p><i>Method: TEMA Section 5 / Kakaç &amp; Liu chapter on vibration. "
      "Effective mass m<sub>e</sub> = tube metal + contained fluid + added-mass "
      "from shell-side fluid (C<sub>m</sub> ≈ 1.5 for submerged tubes).</i></p>"));
  txt->setMaximumHeight(220);
  main->addWidget(txt);

  // --- Close button ------------------------------------------------------
  auto *buttons = new QHBoxLayout();
  buttons->addStretch();
  auto *close = new QPushButton(QStringLiteral("Close"), this);
  buttons->addWidget(close);
  main->addLayout(buttons);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);
}
