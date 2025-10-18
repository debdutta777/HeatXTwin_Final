#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QThread>
#include <QString>
#include <memory>
#include <vector>

#include "core/Thermo.hpp"
#include "core/Hydraulics.hpp"
#include "core/Fouling.hpp"
#include "core/Simulator.hpp"

class ChartWidget;
class SimWorker;
class QGroupBox;

/**
 * @brief Modern redesigned main window with tabbed charts and full UI parameter editing
 * 
 * Features:
 * - 4 separate optimized charts in tabs (Temps, Heat/U, Pressures, Fouling)
 * - All parameters editable directly in UI
 * - Clean, modern, professional design
 * - No external config files needed (but can load/save)
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  enum class SimulationMode {
    SteadyClean = 0,
    SteadyFouling = 1,
    DynamicClean = 2,
    DynamicFouling = 3
  };

private slots:
  void onStart();
  void onStop();
  void onPause();
  void onReset();
  void onZoomIn();
  void onZoomOut();
  void onZoomReset();
  void onExportData();
  void onParameterChanged();
  void onSimulationSample(double t, const hx::State& state);
  void onSimulationFinished();
  void onSimulationModeChanged(int index);

private:
  void setupUi();
  QWidget* createTopBar();
  QWidget* createLeftPanel();
  void createChartTabs();
  void applyModernStyle();
  void updateSimulationCore();
  void resetToDefaults();
  void applySimulationModeToUi();
  static QString simulationModeLabel(SimulationMode mode);

  // === TOP BAR CONTROLS ===
  QPushButton *btnStart_{};
  QPushButton *btnStop_{};
  QPushButton *btnPause_{};
  QPushButton *btnReset_{};
  QPushButton *btnZoomIn_{};
  QPushButton *btnZoomOut_{};
  QPushButton *btnZoomReset_{};
  QPushButton *btnExport_{};
  QLabel *lblStatus_{};
  QDoubleSpinBox *spnDuration_{};
  QDoubleSpinBox *spnTimeStep_{};
  QComboBox *cmbSpeed_{};
  QComboBox *cmbSimulationMode_{};

  // === OPERATING PARAMETERS (LEFT PANEL) ===
  QDoubleSpinBox *spnHotFlowRate_{};
  QDoubleSpinBox *spnColdFlowRate_{};
  QDoubleSpinBox *spnHotInletTemp_{};
  QDoubleSpinBox *spnColdInletTemp_{};

  // === HOT FLUID PROPERTIES ===
  QDoubleSpinBox *spnHotDensity_{};
  QDoubleSpinBox *spnHotViscosity_{};
  QDoubleSpinBox *spnHotSpecificHeat_{};
  QDoubleSpinBox *spnHotConductivity_{};

  // === COLD FLUID PROPERTIES ===
  QDoubleSpinBox *spnColdDensity_{};
  QDoubleSpinBox *spnColdViscosity_{};
  QDoubleSpinBox *spnColdSpecificHeat_{};
  QDoubleSpinBox *spnColdConductivity_{};

  // === GEOMETRY PARAMETERS (LEFT PANEL) ===
  QSpinBox *spnNumTubes_{};
  QDoubleSpinBox *spnTubeDiameter_{};
  QDoubleSpinBox *spnTubeOuterDiameter_{};
  QDoubleSpinBox *spnTubeLength_{};
  QDoubleSpinBox *spnTubePitch_{};
  QDoubleSpinBox *spnShellDiameter_{};
  QDoubleSpinBox *spnBaffleSpacing_{};
  QDoubleSpinBox *spnBaffleCutFraction_{};
  QSpinBox *spnNumBaffles_{};
  QDoubleSpinBox *spnWallConductivity_{};
  QDoubleSpinBox *spnWallThickness_{};

  // === FOULING PARAMETERS (LEFT PANEL) ===
  QDoubleSpinBox *spnRf0_{};
  QDoubleSpinBox *spnRfMax_{};
  QDoubleSpinBox *spnTau_{};
  QDoubleSpinBox *spnAlpha_{};
  QComboBox *cmbFoulingModel_{};
  QGroupBox *grpFouling_{};

  // === LIMITS ===
  QDoubleSpinBox *spnMaxTubePressureDrop_{};
  QDoubleSpinBox *spnMaxShellPressureDrop_{};
  QDoubleSpinBox *spnMinColdFlowRate_{};
  QDoubleSpinBox *spnMaxColdFlowRate_{};

  // === CHART TABS (RIGHT PANEL) ===
  QTabWidget *chartTabs_{};
  ChartWidget *chartTemp_{};       // Temperatures (20-100°C)
  ChartWidget *chartHeat_{};       // Q & U (0-500)
  ChartWidget *chartPressure_{};   // dP_tube & dP_shell (0-40k)
  ChartWidget *chartFouling_{};    // Rf (0-10)

  // === SIMULATION CORE ===
  std::unique_ptr<hx::Thermo> thermo_;
  std::unique_ptr<hx::Hydraulics> hydro_;
  std::unique_ptr<hx::Fouling> fouling_;
  hx::OperatingPoint op_{};
  hx::Geometry geom_{};
  hx::Fluid hot_{};
  hx::Fluid cold_{};
  hx::FoulingParams foulParams_{};
  hx::SimConfig simConfig_{};

  // === BACKGROUND SIMULATION ===
  QThread *simThread_{};
  SimWorker *simWorker_{};
  SimulationMode simulationMode_{SimulationMode::DynamicFouling};
  
  bool isRunning_{false};
  bool isPaused_{false};
  std::vector<std::pair<double, hx::State>> simulationData_;  // For export
};

