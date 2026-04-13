#include "KPIPanel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <algorithm>
#include <cmath>

namespace {

// Use a shared colour palette matching the rest of the app.
const QString kCardStyle =
    "QFrame { background: #ffffff; border: 1px solid #d0d7de;"
    "        border-radius: 8px; padding: 10px; }"
    "QLabel#kpiTitle   { color: #1f6feb; font-weight: 700; font-size: 10pt; }"
    "QLabel#kpiValue   { color: #0b1f44; font-weight: 800; font-size: 20pt;"
    "                    font-family: 'Segoe UI Semibold','Segoe UI',sans-serif; }"
    "QLabel#kpiFormula { color: #57606a; font-size: 9pt;"
    "                    font-family: 'Cambria Math','Cambria',serif;"
    "                    font-style: italic; }"
    "QLabel#kpiStatus  { font-size: 8pt; font-weight: 600; padding-top: 2px; }";

const QString kStatusGood = "color: #1a7f37;";
const QString kStatusWarn = "color: #bf8700;";
const QString kStatusBad  = "color: #cf222e;";

QString fmtNum(double v, int digits = 3) {
  if (!std::isfinite(v)) return QStringLiteral("—");
  return QString::number(v, 'f', digits);
}

}  // namespace

KPIPanel::KPIPanel(QWidget *parent) : QWidget(parent) {
  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setStyleSheet("QScrollArea { background: #f5f7fa; }");

  auto *content = new QWidget();
  content->setStyleSheet("background: #f5f7fa;");

  auto *header = new QLabel(
      "<b>Key Performance Indicators</b> &nbsp;·&nbsp; "
      "<span style='color:#57606a;'>live metrics derived from simulation state</span>",
      content);
  header->setStyleSheet("font-size: 12pt; padding: 8px 4px;");
  header->setTextFormat(Qt::RichText);

  auto *grid = new QGridLayout();
  grid->setSpacing(12);
  grid->setContentsMargins(8, 4, 8, 8);

  qCard_ = makeCard(
      "Heat Duty (Q)",
      "Q&nbsp;=&nbsp;m&#775;<sub>h</sub>&nbsp;c<sub>p,h</sub>&nbsp;(T<sub>h,in</sub>&nbsp;&minus;&nbsp;T<sub>h,out</sub>)",
      "kW");

  uCard_ = makeCard(
      "Overall U",
      "1/U&nbsp;=&nbsp;1/h<sub>s</sub>&nbsp;+&nbsp;R<sub>w</sub>&nbsp;+&nbsp;(D<sub>o</sub>/D<sub>i</sub>)/h<sub>t</sub>&nbsp;+&nbsp;R<sub>f,s</sub>&nbsp;+&nbsp;R<sub>f,t</sub>",
      "W/m\xC2\xB2\xC2\xB7K");

  effectivenessCard_ = makeCard(
      "Effectiveness (\xCE\xB5)",
      "\xCE\xB5&nbsp;=&nbsp;Q&nbsp;/&nbsp;Q<sub>max</sub>&nbsp;=&nbsp;Q&nbsp;/&nbsp;[C<sub>min</sub>(T<sub>h,in</sub>&nbsp;&minus;&nbsp;T<sub>c,in</sub>)]",
      "—");

  ntuCard_ = makeCard(
      "NTU",
      "NTU&nbsp;=&nbsp;UA&nbsp;/&nbsp;C<sub>min</sub>",
      "—");

  crCard_ = makeCard(
      "Capacity Ratio (C<sub>r</sub>)",
      "C<sub>r</sub>&nbsp;=&nbsp;C<sub>min</sub>&nbsp;/&nbsp;C<sub>max</sub>",
      "—");

  recoveryCard_ = makeCard(
      "Heat Recovery",
      "&eta;<sub>rec</sub>&nbsp;=&nbsp;(T<sub>c,out</sub>&nbsp;&minus;&nbsp;T<sub>c,in</sub>)&nbsp;/&nbsp;(T<sub>h,in</sub>&nbsp;&minus;&nbsp;T<sub>c,in</sub>)",
      "%");

  foulingPenaltyCard_ = makeCard(
      "Fouling Penalty",
      "\xCE\xB4<sub>U</sub>&nbsp;=&nbsp;(U<sub>clean</sub>&nbsp;&minus;&nbsp;U)&nbsp;/&nbsp;U<sub>clean</sub>",
      "%");

  tubeDpMarginCard_ = makeCard(
      "Tube \xCE\x94P Margin",
      "margin&nbsp;=&nbsp;1&nbsp;&minus;&nbsp;\xCE\x94P<sub>tube</sub>&nbsp;/&nbsp;\xCE\x94P<sub>tube,max</sub>",
      "%");

  shellDpMarginCard_ = makeCard(
      "Shell \xCE\x94P Margin",
      "margin&nbsp;=&nbsp;1&nbsp;&minus;&nbsp;\xCE\x94P<sub>shell</sub>&nbsp;/&nbsp;\xCE\x94P<sub>shell,max</sub>",
      "%");

  // Arrange: 3 columns x 3 rows
  grid->addWidget(qCard_.container,              0, 0);
  grid->addWidget(uCard_.container,              0, 1);
  grid->addWidget(effectivenessCard_.container,  0, 2);
  grid->addWidget(ntuCard_.container,            1, 0);
  grid->addWidget(crCard_.container,             1, 1);
  grid->addWidget(recoveryCard_.container,       1, 2);
  grid->addWidget(foulingPenaltyCard_.container, 2, 0);
  grid->addWidget(tubeDpMarginCard_.container,   2, 1);
  grid->addWidget(shellDpMarginCard_.container,  2, 2);

  auto *footer = new QLabel(
      "<i>C<sub>min</sub>, C<sub>max</sub> are the smaller/larger of "
      "m&#775;<sub>h</sub> c<sub>p,h</sub> and m&#775;<sub>c</sub> c<sub>p,c</sub>. "
      "Q<sub>max</sub> is the thermodynamic upper bound on heat transfer.</i>",
      content);
  footer->setStyleSheet("color: #57606a; font-size: 9pt; padding: 4px;");
  footer->setTextFormat(Qt::RichText);
  footer->setWordWrap(true);

  auto *outer = new QVBoxLayout(content);
  outer->addWidget(header);
  outer->addLayout(grid);
  outer->addWidget(footer);
  outer->addStretch(1);

  scroll->setWidget(content);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(scroll);

  reset();
}

KPIPanel::Card KPIPanel::makeCard(const QString &title,
                                    const QString &formulaHtml,
                                    const QString &unit) {
  Card card;
  card.container = new QFrame(this);
  card.container->setStyleSheet(kCardStyle);

  auto *lay = new QVBoxLayout(card.container);
  lay->setSpacing(4);
  lay->setContentsMargins(12, 10, 12, 10);

  card.title = new QLabel(title, card.container);
  card.title->setObjectName("kpiTitle");
  card.title->setTextFormat(Qt::RichText);

  card.value = new QLabel("—", card.container);
  card.value->setObjectName("kpiValue");

  auto *unitLabel = new QLabel(unit, card.container);
  unitLabel->setStyleSheet("color: #57606a; font-size: 9pt;");

  auto *valueRow = new QHBoxLayout();
  valueRow->addWidget(card.value);
  valueRow->addWidget(unitLabel, 0, Qt::AlignBottom);
  valueRow->addStretch(1);

  card.formula = new QLabel(formulaHtml, card.container);
  card.formula->setObjectName("kpiFormula");
  card.formula->setTextFormat(Qt::RichText);
  card.formula->setWordWrap(true);

  card.status = new QLabel("awaiting simulation", card.container);
  card.status->setObjectName("kpiStatus");
  card.status->setStyleSheet(kStatusWarn);

  lay->addWidget(card.title);
  lay->addLayout(valueRow);
  lay->addWidget(card.formula);
  lay->addWidget(card.status);

  return card;
}

void KPIPanel::setCardValue(Card &card,
                              double value,
                              const QString &fmt,
                              double goodLow,
                              double goodHigh) {
  if (!std::isfinite(value)) {
    card.value->setText("—");
    card.status->setText("n/a");
    card.status->setStyleSheet(kStatusWarn);
    return;
  }
  card.value->setText(QString::number(value, 'f', fmt.toInt()));

  if (value >= goodLow && value <= goodHigh) {
    card.status->setText("healthy");
    card.status->setStyleSheet(kStatusGood);
  } else if (value < goodLow * 0.6 || value > goodHigh * 1.2) {
    card.status->setText("critical");
    card.status->setStyleSheet(kStatusBad);
  } else {
    card.status->setText("monitor");
    card.status->setStyleSheet(kStatusWarn);
  }
}

void KPIPanel::reset() {
  auto clearCard = [](Card &c) {
    c.value->setText("—");
    c.status->setText("awaiting simulation");
    c.status->setStyleSheet(kStatusWarn);
  };
  clearCard(qCard_);
  clearCard(uCard_);
  clearCard(effectivenessCard_);
  clearCard(ntuCard_);
  clearCard(crCard_);
  clearCard(recoveryCard_);
  clearCard(foulingPenaltyCard_);
  clearCard(tubeDpMarginCard_);
  clearCard(shellDpMarginCard_);
}

void KPIPanel::update(const hx::State &state,
                       const hx::OperatingPoint &op,
                       const hx::Fluid &hot,
                       const hx::Fluid &cold,
                       const hx::Geometry &geom,
                       const hx::Limits &limits,
                       double U_clean) {
  const double Ch = op.m_dot_hot  * hot.cp;
  const double Cc = op.m_dot_cold * cold.cp;
  const double Cmin = std::min(Ch, Cc);
  const double Cmax = std::max(Ch, Cc);
  const double dT_in = op.Tin_hot - op.Tin_cold;
  const double Qmax = (Cmin > 0 && dT_in > 0) ? Cmin * dT_in : 0.0;
  const double UA = state.U * geom.areaOuter();

  // 1) Q (kW)
  qCard_.value->setText(fmtNum(state.Q / 1000.0, 2));
  qCard_.status->setText(state.Q > 0 ? "transferring" : "idle");
  qCard_.status->setStyleSheet(state.Q > 0 ? kStatusGood : kStatusWarn);

  // 2) U (W/m²K)
  uCard_.value->setText(fmtNum(state.U, 1));
  if (state.U > 800)        { uCard_.status->setText("high");      uCard_.status->setStyleSheet(kStatusGood); }
  else if (state.U > 300)   { uCard_.status->setText("nominal");   uCard_.status->setStyleSheet(kStatusGood); }
  else if (state.U > 100)   { uCard_.status->setText("degraded");  uCard_.status->setStyleSheet(kStatusWarn); }
  else                       { uCard_.status->setText("poor");      uCard_.status->setStyleSheet(kStatusBad);  }

  // 3) ε
  const double eps = (Qmax > 0) ? std::clamp(state.Q / Qmax, 0.0, 1.0)
                                : std::numeric_limits<double>::quiet_NaN();
  effectivenessCard_.value->setText(fmtNum(eps, 3));
  if (std::isfinite(eps)) {
    if      (eps >= 0.7) { effectivenessCard_.status->setText("excellent"); effectivenessCard_.status->setStyleSheet(kStatusGood); }
    else if (eps >= 0.5) { effectivenessCard_.status->setText("acceptable"); effectivenessCard_.status->setStyleSheet(kStatusGood); }
    else if (eps >= 0.3) { effectivenessCard_.status->setText("low");        effectivenessCard_.status->setStyleSheet(kStatusWarn); }
    else                 { effectivenessCard_.status->setText("critical");   effectivenessCard_.status->setStyleSheet(kStatusBad);  }
  }

  // 4) NTU
  const double ntu = (Cmin > 0) ? UA / Cmin : std::numeric_limits<double>::quiet_NaN();
  ntuCard_.value->setText(fmtNum(ntu, 3));
  if (std::isfinite(ntu)) {
    if      (ntu >= 3.0) { ntuCard_.status->setText("high");    ntuCard_.status->setStyleSheet(kStatusGood); }
    else if (ntu >= 1.0) { ntuCard_.status->setText("nominal"); ntuCard_.status->setStyleSheet(kStatusGood); }
    else                 { ntuCard_.status->setText("low");     ntuCard_.status->setStyleSheet(kStatusWarn); }
  }

  // 5) Cr
  const double cr = (Cmax > 0) ? Cmin / Cmax : std::numeric_limits<double>::quiet_NaN();
  crCard_.value->setText(fmtNum(cr, 3));
  if (std::isfinite(cr)) {
    crCard_.status->setText(cr < 0.95 ? "unbalanced" : "balanced");
    crCard_.status->setStyleSheet(kStatusGood);
  }

  // 6) Heat recovery (cold-side Δ over inlet ΔT)
  const double recovery = (dT_in > 0)
      ? std::clamp((state.Tc_out - op.Tin_cold) / dT_in, 0.0, 1.0)
      : std::numeric_limits<double>::quiet_NaN();
  recoveryCard_.value->setText(std::isfinite(recovery) ? fmtNum(recovery * 100, 1) : QStringLiteral("—"));
  if (std::isfinite(recovery)) {
    if      (recovery >= 0.6) { recoveryCard_.status->setText("excellent");  recoveryCard_.status->setStyleSheet(kStatusGood); }
    else if (recovery >= 0.4) { recoveryCard_.status->setText("acceptable"); recoveryCard_.status->setStyleSheet(kStatusGood); }
    else                      { recoveryCard_.status->setText("poor");       recoveryCard_.status->setStyleSheet(kStatusWarn); }
  }

  // 7) Fouling penalty
  const double penalty = (U_clean > 0)
      ? std::clamp((U_clean - state.U) / U_clean, 0.0, 1.0)
      : std::numeric_limits<double>::quiet_NaN();
  foulingPenaltyCard_.value->setText(std::isfinite(penalty) ? fmtNum(penalty * 100, 1) : QStringLiteral("—"));
  if (std::isfinite(penalty)) {
    if      (penalty < 0.05) { foulingPenaltyCard_.status->setText("clean");    foulingPenaltyCard_.status->setStyleSheet(kStatusGood); }
    else if (penalty < 0.20) { foulingPenaltyCard_.status->setText("fouling");  foulingPenaltyCard_.status->setStyleSheet(kStatusWarn); }
    else                     { foulingPenaltyCard_.status->setText("severe");   foulingPenaltyCard_.status->setStyleSheet(kStatusBad);  }
  }

  // 8) Tube dP margin
  const double tubeMargin = (limits.dP_tube_max > 0)
      ? std::clamp(1.0 - state.dP_tube / limits.dP_tube_max, -1.0, 1.0)
      : std::numeric_limits<double>::quiet_NaN();
  tubeDpMarginCard_.value->setText(std::isfinite(tubeMargin) ? fmtNum(tubeMargin * 100, 1) : QStringLiteral("—"));
  if (std::isfinite(tubeMargin)) {
    if      (tubeMargin >= 0.3) { tubeDpMarginCard_.status->setText("safe");      tubeDpMarginCard_.status->setStyleSheet(kStatusGood); }
    else if (tubeMargin >= 0.0) { tubeDpMarginCard_.status->setText("near limit"); tubeDpMarginCard_.status->setStyleSheet(kStatusWarn); }
    else                        { tubeDpMarginCard_.status->setText("exceeded");  tubeDpMarginCard_.status->setStyleSheet(kStatusBad);  }
  }

  // 9) Shell dP margin
  const double shellMargin = (limits.dP_shell_max > 0)
      ? std::clamp(1.0 - state.dP_shell / limits.dP_shell_max, -1.0, 1.0)
      : std::numeric_limits<double>::quiet_NaN();
  shellDpMarginCard_.value->setText(std::isfinite(shellMargin) ? fmtNum(shellMargin * 100, 1) : QStringLiteral("—"));
  if (std::isfinite(shellMargin)) {
    if      (shellMargin >= 0.3) { shellDpMarginCard_.status->setText("safe");       shellDpMarginCard_.status->setStyleSheet(kStatusGood); }
    else if (shellMargin >= 0.0) { shellDpMarginCard_.status->setText("near limit"); shellDpMarginCard_.status->setStyleSheet(kStatusWarn); }
    else                         { shellDpMarginCard_.status->setText("exceeded");   shellDpMarginCard_.status->setStyleSheet(kStatusBad);  }
  }

  // Silence unused-parameter warnings on variables only used via fmt helpers.
  (void)geom;
}
