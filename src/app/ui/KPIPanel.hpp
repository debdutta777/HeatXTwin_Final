#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include "core/Types.hpp"

/**
 * \brief Key Performance Indicators panel — educational dashboard.
 *
 * Displays textbook-quality derived metrics from the live simulation state:
 *   - Effectiveness            ε  = Q / Q_max
 *   - NTU                      NTU = UA / C_min
 *   - Capacity ratio           C_r = C_min / C_max
 *   - Heat recovery            η  = Q / (m_c c_p,c (T_h,in − T_c,in))
 *   - Fouling penalty          (U_clean − U) / U_clean
 *   - Tube dP margin           (dP_max − dP) / dP_max
 *   - Shell dP margin          (dP_max − dP) / dP_max
 *
 * Each metric is rendered with its formula (HTML with Unicode super/subscripts
 * and Greek letters) so the panel doubles as a quick-reference card.
 */
class KPIPanel : public QWidget {
  Q_OBJECT
public:
  explicit KPIPanel(QWidget *parent = nullptr);

  /** Update all KPIs from the current simulation sample. */
  void update(const hx::State &state,
              const hx::OperatingPoint &op,
              const hx::Fluid &hot,
              const hx::Fluid &cold,
              const hx::Geometry &geom,
              const hx::Limits &limits,
              double U_clean);

  /** Reset all values to n/a. */
  void reset();

private:
  struct Card {
    QLabel *title{};
    QLabel *value{};
    QLabel *formula{};
    QLabel *status{};
    QWidget *container{};
  };

  Card makeCard(const QString &title,
                const QString &formulaHtml,
                const QString &unit);

  void setCardValue(Card &card,
                    double value,
                    const QString &fmt,
                    double goodLow,
                    double goodHigh);

  Card effectivenessCard_;
  Card ntuCard_;
  Card crCard_;
  Card recoveryCard_;
  Card foulingPenaltyCard_;
  Card tubeDpMarginCard_;
  Card shellDpMarginCard_;
  Card qCard_;
  Card uCard_;
};
