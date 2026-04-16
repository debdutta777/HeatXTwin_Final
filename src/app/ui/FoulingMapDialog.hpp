#pragma once

#include <QDialog>
#include "core/FoulingMap.hpp"
#include "core/Types.hpp"

class QLabel;

/**
 *  \brief Per-tube fouling heatmap dialog.
 *
 *  Shows three coordinated views of the same \c FoulingMap:
 *    1. **Bundle plan-view** — each tube rendered as a coloured circle using
 *       a viridis palette, so centre-vs-wall and window-vs-crossflow tubes
 *       separate visually.
 *    2. **Axial strip** — Rf averaged across the bundle vs axial position,
 *       demonstrating the downstream build-up γ·z/L term in the model.
 *    3. **Tooltip/readout** — mean, min, max, hot-spot fraction, and the
 *       physics legend (A_r, A_w, γ, shear factor).
 */
class QLabel;

class FoulingMapDialog : public QDialog {
  Q_OBJECT
public:
  FoulingMapDialog(const hx::FoulingMap &map,
                    const hx::Geometry  &geom,
                    QWidget *parent = nullptr);

public slots:
  /** Replace the displayed map/geometry and trigger a full repaint of the
   *  three child views plus the summary cards.  Called by MainWindow while
   *  a simulation is running so the dialog reflects the live fouling field
   *  rather than the t=0 snapshot it was opened with. */
  void updateMap(const hx::FoulingMap &map, const hx::Geometry &geom);

private:
  void refreshSummaries();

  // Own the data: child widgets hold references that must outlive the caller.
  // The caller's FoulingMap is typically a local in the slot that created
  // the dialog and would otherwise dangle once the slot returns.
  hx::FoulingMap map_;
  hx::Geometry   geom_;

  // Cached widgets we mutate on live updates.
  QLabel *header_{};
  QLabel *lblMeanOverall_{};
  QLabel *lblMin_{};
  QLabel *lblMax_{};
  QLabel *lblRatio_{};
  QLabel *lblHotSpot_{};
  QWidget *bundleView_{};
  QWidget *axialStrip_{};
  QWidget *colorBar_{};
};
