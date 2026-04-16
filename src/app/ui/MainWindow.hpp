#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QThread>
#include <QTimer>
#include <QString>
#include <memory>
#include <vector>

#include "core/Thermo.hpp"
#include "core/Hydraulics.hpp"
#include "core/Fouling.hpp"
#include "core/FluidLibrary.hpp"
#include "core/Simulator.hpp"

class ChartWidget;
class HeatExchangerWidget;
class KPIPanel;
class SpectrumWidget;
class SimWorker;
class QGroupBox;
class FoulingMapDialog;

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
  void onSnapshotBaseline();
  void onClearBaseline();
  void onGenerateReport();
  void onAutoTunePid();
  void onMonteCarlo();
  void onVibrationCheck();
  void onFoulingHeatmap();
  void onRunLog();
  void onParameterChanged();
  void onGeometryDebounceTimeout();
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
  void validateAndUpdateGeometry();
  static QString simulationModeLabel(SimulationMode mode);
  static bool validateGeometryParameters(const hx::Geometry &geom, QString &errorMsg);

  // === TOP BAR CONTROLS ===
  QPushButton *btnStart_{};
  QPushButton *btnStop_{};
  QPushButton *btnPause_{};
  QPushButton *btnReset_{};
  QPushButton *btnZoomIn_{};
  QPushButton *btnZoomOut_{};
  QPushButton *btnZoomReset_{};
  QPushButton *btnExport_{};
  QPushButton *btnSnapshot_{};
  QPushButton *btnClearBaseline_{};
  QPushButton *btnReport_{};
  QPushButton *btnMonteCarlo_{};
  QPushButton *btnVibration_{};
  QPushButton *btnHeatmap_{};
  QPushButton *btnRunLog_{};
  QLabel *lblStatus_{};
  QDoubleSpinBox *spnDuration_{};
  QDoubleSpinBox *spnTimeStep_{};
  QComboBox *cmbSpeed_{};
  QComboBox *cmbSimulationMode_{};
  QComboBox *cmbScenario_{};

  // === OPERATING PARAMETERS (LEFT PANEL) ===
  QDoubleSpinBox *spnHotFlowRate_{};
  QDoubleSpinBox *spnColdFlowRate_{};
  QDoubleSpinBox *spnHotInletTemp_{};
  QDoubleSpinBox *spnColdInletTemp_{};

  // === HOT FLUID PROPERTIES ===
  QComboBox *cmbHotPreset_{};
  QDoubleSpinBox *spnHotDensity_{};
  QDoubleSpinBox *spnHotViscosity_{};
  QDoubleSpinBox *spnHotSpecificHeat_{};
  QDoubleSpinBox *spnHotConductivity_{};

  // === COLD FLUID PROPERTIES ===
  QComboBox *cmbColdPreset_{};
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
  QComboBox *cmbFlowArrangement_{};
  QComboBox *cmbShellMethod_{};

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

  // === PID CONTROL ===
  QGroupBox *grpPid_{};
  QDoubleSpinBox *spnPidSetpoint_{};
  QDoubleSpinBox *spnPidKp_{};
  QDoubleSpinBox *spnPidKi_{};
  QDoubleSpinBox *spnPidKd_{};
  QDoubleSpinBox *spnPidUMin_{};
  QDoubleSpinBox *spnPidUMax_{};

  // Feed-forward + cascade (B5)
  QCheckBox      *chkFFEnabled_{};
  QCheckBox      *chkFFAutoEB_{};
  QDoubleSpinBox *spnFFkTin_{};
  QDoubleSpinBox *spnFFkFlow_{};
  QCheckBox      *chkCascadeEnabled_{};
  QDoubleSpinBox *spnTauValve_{};

  // === CHART TABS (RIGHT PANEL) ===
  QTabWidget *chartTabs_{};
  ChartWidget *chartTemp_{};       // Temperatures (20-100°C)
  ChartWidget *chartHeat_{};       // Q & U (0-500)
  ChartWidget *chartPressure_{};   // dP_tube & dP_shell (0-40k)
  ChartWidget *chartFouling_{};    // Rf (0-10)
  ChartWidget *chartPID_{};        // PID setpoint tracking + cold-flow command
  HeatExchangerWidget *exchWidget_{};
  KPIPanel *kpiPanel_{};
  SpectrumWidget *spectrumWidget_{};
  double U_clean_baseline_{0.0};   // Captured at simulation start for fouling penalty

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
  
  // === GEOMETRY UPDATE DEBOUNCING ===
  QTimer *geometryUpdateTimer_{};
  
  bool isRunning_{false};
  bool isPaused_{false};
  std::vector<std::pair<double, hx::State>> simulationData_;  // For export

  // Live per-tube fouling heatmap dialog (non-modal).  When open, simulation
  // samples push fresh FoulingMap snapshots through updateMap() so the user
  // sees fouling evolve in real time instead of a frozen t=0 picture.
  FoulingMapDialog *heatmapDlg_{};
  int               heatmapSampleCounter_{0};
};

