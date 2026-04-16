#pragma once

#include <QDialog>
#include "core/VibrationCheck.hpp"

/**
 * \brief TEMA flow-induced vibration diagnostic dialog.
 *
 *  Three status lights (green/yellow/red) for vortex-shedding,
 *  fluid-elastic instability, and acoustic resonance, plus a formulae
 *  table showing all intermediate quantities (fn, fv, V_crit, etc.).
 */
class VibrationDialog : public QDialog {
  Q_OBJECT
public:
  VibrationDialog(const hx::VibrationResult &result,
                   const hx::VibrationConfig &config,
                   QWidget *parent = nullptr);
};
