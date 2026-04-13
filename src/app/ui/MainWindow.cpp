#include "MainWindow.hpp"
#include "ChartWidget.hpp"
#include "HeatExchangerWidget.hpp"
#include "KPIPanel.hpp"
#include "SimWorker.hpp"
#include "Diagnostics.hpp"
#include "core/Simulator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QScrollArea>
#include <QFrame>
#include <QTextStream>
#include <QFile>
#include <QSignalBlocker>
#include <QString>
#include <QApplication>
#include <QDateTime>
#include <QLocale>
#include <QDesktopServices>
#include <QUrl>
#include <cmath>
#include <algorithm>

namespace {
struct ModeConfig {
  bool steady;
  bool fouling;
};

ModeConfig modeConfig(MainWindow::SimulationMode mode) {
  using Mode = MainWindow::SimulationMode;
  switch (mode) {
    case Mode::SteadyClean:
      return {true, false};
    case Mode::SteadyFouling:
      return {true, true};
    case Mode::DynamicClean:
      return {false, false};
    case Mode::DynamicFouling:
    default:
      return {false, true};
  }
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // Initialize diagnostics
  auto diagnostics = std::make_unique<Diagnostics>();
  
  DIAG_BEGIN(*diagnostics, "Application Startup");
  setWindowTitle("HeatXTwin Pro - Modern Heat Exchanger Digital Twin");
  resize(1400, 900);
  
  // Setup geometry update debouncing (300ms delay for rapid parameter changes)
  DIAG_BEGIN(*diagnostics, "Initialize Timer");
  geometryUpdateTimer_ = new QTimer(this);
  geometryUpdateTimer_->setSingleShot(true);
  geometryUpdateTimer_->setInterval(300);
  connect(geometryUpdateTimer_, &QTimer::timeout, this, &MainWindow::onGeometryDebounceTimeout);
  DIAG_END(*diagnostics, "Debounce timer ready");
  
  DIAG_BEGIN(*diagnostics, "Reset to Defaults");
  resetToDefaults();  // Initialize with default values
  DIAG_END(*diagnostics, "Default parameters loaded");
  
  DIAG_BEGIN(*diagnostics, "Setup UI");
  setupUi();
  DIAG_END(*diagnostics, "UI widgets created");
  
  DIAG_BEGIN(*diagnostics, "Apply Simulation Mode");
  applySimulationModeToUi();
  DIAG_END(*diagnostics, "Simulation mode configured");
  
  DIAG_BEGIN(*diagnostics, "Apply Modern Style");
  applyModernStyle();
  DIAG_END(*diagnostics, "Styling applied");
  
  DIAG_BEGIN(*diagnostics, "Update Simulation Core");
  updateSimulationCore();
  DIAG_END(*diagnostics, "Core simulation objects created");
  
  // Remove margins from central widget for more space
  centralWidget()->setContentsMargins(0, 0, 0, 0);
  
  // Run final diagnostic checks
  diagnostics->runAllChecks();
  
  // Export diagnostics
  QString appDir = QApplication::applicationDirPath();
  diagnostics->exportToLog(appDir + "/Startup.log");
  diagnostics->exportToJson(appDir + "/Startup.json");
  diagnostics->printSummary();
  DIAG_END(*diagnostics, "Diagnostics complete");
}

MainWindow::~MainWindow() {
  if (simThread_ && simThread_->isRunning()) {
    simWorker_->stop();
    simThread_->quit();
    simThread_->wait();
  }
}

void MainWindow::resetToDefaults() {
  // Operating point defaults
  op_.m_dot_hot = 1.20;
  op_.m_dot_cold = 1.00;
  op_.Tin_hot = 80.0;
  op_.Tin_cold = 25.0;

  // Geometry defaults
  geom_.nTubes = 100;
  geom_.Di = 0.019;       // 19mm
  geom_.Do = 0.025;       // 25mm
  geom_.L = 5.0;          // 5m
  geom_.pitch = 0.032;
  geom_.shellID = 0.5;    // 500mm
  geom_.baffleSpacing = 0.25;  // 250mm
  geom_.baffleCutFrac = 0.25;
  geom_.nBaffles = 20;
  geom_.wall_k = 16.0;
  geom_.wall_thickness = 0.002;
  geom_.K_minor_tube = 1.5;      // Default minor loss
  geom_.K_turns_shell = 0.0;     // Default (calculated)

  // Fluid properties (water)
  hot_.rho = 997.0;
  hot_.mu = 0.001;
  hot_.cp = 4180.0;
  hot_.k = 0.6;
  
  cold_ = hot_;  // Same fluid properties

  // Fouling parameters
  foulParams_.Rf0 = 0.0;
  foulParams_.RfMax = 0.0005;
  foulParams_.tau = 3600.0;
  foulParams_.alpha = 1e-8;
  foulParams_.model = hx::FoulingParams::Model::Asymptotic;
  foulParams_.k_deposit = 0.5; // Default conductivity
  foulParams_.split_ratio = 0.5; // Default 50/50 split

  // Simulation config
  simConfig_.dt = 0.1;
  simConfig_.tEnd = 3600.0;
  simConfig_.disturbanceType = hx::SimConfig::DisturbanceType::SineWave; // Default disturbance
  
  // Dynamic simulation parameters (holdup masses)
  // Typical values: ~10kg hot side, ~12kg cold side for moderate exchanger
  simConfig_.Mh = 10.0;  // [kg] hot-side fluid holdup mass
  simConfig_.Mc = 12.0;  // [kg] cold-side fluid holdup mass
}

void MainWindow::setupUi() {
  auto *central = new QWidget(this);
  setCentralWidget(central);
  
  auto *mainLayout = new QVBoxLayout(central);
  mainLayout->setSpacing(0);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // Top bar with controls - more compact
  auto *topBar = createTopBar();
  mainLayout->addWidget(topBar);

  // Main content: Parameters (left) + Charts (right)
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setHandleWidth(2);
  
  auto *leftPanel = createLeftPanel();
  splitter->addWidget(leftPanel);
  
  createChartTabs();
  splitter->addWidget(chartTabs_);
  
  // Set initial sizes: left panel 500px, rest for charts
  splitter->setSizes(QList<int>() << 500 << 900);
  splitter->setStretchFactor(0, 0);  // Left panel: fixed-ish
  splitter->setStretchFactor(1, 1);  // Charts: take remaining space
  
  mainLayout->addWidget(splitter);

  // Status bar
  statusBar()->showMessage("Ready");
}

QWidget* MainWindow::createTopBar() {
  auto *topBar = new QWidget(this);
  topBar->setMinimumHeight(110);
  topBar->setMaximumHeight(115);
  topBar->setStyleSheet(R"(
    QWidget {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                  stop:0 #f8f9fa, stop:1 #e9ecef);
      border-bottom: 3px solid #3498db;
    }
  )");

  auto *mainLayout = new QVBoxLayout(topBar);
  mainLayout->setSpacing(5);
  mainLayout->setContentsMargins(10, 5, 10, 5);

  // First row - Control buttons and chart controls
  auto *row1Layout = new QHBoxLayout();
  row1Layout->setSpacing(10);

  // Second row - Parameters
  auto *row2Layout = new QHBoxLayout();
  row2Layout->setSpacing(10);

  mainLayout->addLayout(row1Layout);
  mainLayout->addLayout(row2Layout);

  // === SIMULATION CONTROL BUTTONS === (Row 1)
  btnStart_ = new QPushButton("Start", this);
  btnStart_->setObjectName("startButton");
  btnStart_->setMinimumSize(95, 45);
  btnStart_->setMaximumSize(115, 45);
  btnStart_->setToolTip("Start simulation");
  btnStart_->setStyleSheet("font-size: 11pt; font-weight: bold; background-color: #27ae60; color: white;");
  connect(btnStart_, &QPushButton::clicked, this, &MainWindow::onStart);
  row1Layout->addWidget(btnStart_);

  btnPause_ = new QPushButton("Pause", this);
  btnPause_->setObjectName("pauseButton");
  btnPause_->setMinimumSize(95, 45);
  btnPause_->setMaximumSize(115, 45);
  btnPause_->setEnabled(false);
  btnPause_->setToolTip("Pause/Resume simulation");
  btnPause_->setStyleSheet("font-size: 11pt; font-weight: bold; background-color: #f39c12; color: white;");
  connect(btnPause_, &QPushButton::clicked, this, &MainWindow::onPause);
  row1Layout->addWidget(btnPause_);

  btnStop_ = new QPushButton("Stop", this);
  btnStop_->setObjectName("stopButton");
  btnStop_->setMinimumSize(95, 45);
  btnStop_->setMaximumSize(115, 45);
  btnStop_->setEnabled(false);
  btnStop_->setToolTip("Stop simulation");
  btnStop_->setStyleSheet("font-size: 11pt; font-weight: bold; background-color: #e74c3c; color: white;");
  connect(btnStop_, &QPushButton::clicked, this, &MainWindow::onStop);
  row1Layout->addWidget(btnStop_);

  btnReset_ = new QPushButton("Reset", this);
  btnReset_->setMinimumSize(85, 45);
  btnReset_->setMaximumSize(105, 45);
  btnReset_->setToolTip("Reset to default parameters");
  btnReset_->setStyleSheet("font-size: 10pt; font-weight: bold;");
  connect(btnReset_, &QPushButton::clicked, this, &MainWindow::onReset);
  row1Layout->addWidget(btnReset_);

  // Separator
  auto *line1 = new QFrame(this);
  line1->setFrameShape(QFrame::VLine);
  line1->setFrameShadow(QFrame::Sunken);
  row1Layout->addWidget(line1);

  // === CHART CONTROL BUTTONS === (Row 1)
  btnZoomIn_ = new QPushButton("Zoom In", this);
  btnZoomIn_->setMinimumSize(90, 38);
  btnZoomIn_->setToolTip("Zoom in on chart");
  connect(btnZoomIn_, &QPushButton::clicked, this, &MainWindow::onZoomIn);
  row1Layout->addWidget(btnZoomIn_);

  btnZoomOut_ = new QPushButton("Zoom Out", this);
  btnZoomOut_->setMinimumSize(90, 38);
  btnZoomOut_->setToolTip("Zoom out on chart");
  connect(btnZoomOut_, &QPushButton::clicked, this, &MainWindow::onZoomOut);
  row1Layout->addWidget(btnZoomOut_);

  btnZoomReset_ = new QPushButton("Reset Zoom", this);
  btnZoomReset_->setMinimumSize(90, 38);
  btnZoomReset_->setToolTip("Reset zoom to fit all data");
  connect(btnZoomReset_, &QPushButton::clicked, this, &MainWindow::onZoomReset);
  row1Layout->addWidget(btnZoomReset_);

  // Separator
  auto *line2 = new QFrame(this);
  line2->setFrameShape(QFrame::VLine);
  line2->setFrameShadow(QFrame::Sunken);
  row1Layout->addWidget(line2);

  // === EXPORT BUTTON === (Row 1)
  btnExport_ = new QPushButton("Export CSV", this);
  btnExport_->setMinimumSize(100, 38);
  btnExport_->setEnabled(false);
  btnExport_->setToolTip("Export simulation data to CSV");
  connect(btnExport_, &QPushButton::clicked, this, &MainWindow::onExportData);
  row1Layout->addWidget(btnExport_);

  // === SNAPSHOT / BASELINE === (Row 1)
  btnSnapshot_ = new QPushButton("Snapshot", this);
  btnSnapshot_->setMinimumSize(100, 38);
  btnSnapshot_->setEnabled(false);
  btnSnapshot_->setToolTip(
      "Capture the current traces as a baseline overlay.\n"
      "Subsequent runs render on top of the dashed baseline for comparison.");
  connect(btnSnapshot_, &QPushButton::clicked, this, &MainWindow::onSnapshotBaseline);
  row1Layout->addWidget(btnSnapshot_);

  btnClearBaseline_ = new QPushButton("Clear Baseline", this);
  btnClearBaseline_->setMinimumSize(110, 38);
  btnClearBaseline_->setEnabled(false);
  btnClearBaseline_->setToolTip("Remove the baseline overlay from all charts.");
  connect(btnClearBaseline_, &QPushButton::clicked, this, &MainWindow::onClearBaseline);
  row1Layout->addWidget(btnClearBaseline_);

  btnReport_ = new QPushButton("Report (HTML)", this);
  btnReport_->setMinimumSize(110, 38);
  btnReport_->setEnabled(false);
  btnReport_->setToolTip("Generate an HTML report summarising parameters, KPIs and final state.");
  connect(btnReport_, &QPushButton::clicked, this, &MainWindow::onGenerateReport);
  row1Layout->addWidget(btnReport_);

  row1Layout->addStretch();

  // === SIMULATION PARAMETERS === (Row 2)
  row2Layout->addWidget(new QLabel("Duration:", this));
  spnDuration_ = new QDoubleSpinBox(this);
  spnDuration_->setRange(100, 10000);
  spnDuration_->setValue(1800);
  spnDuration_->setSuffix(" s");
  spnDuration_->setMinimumWidth(100);
  row2Layout->addWidget(spnDuration_);

  row2Layout->addWidget(new QLabel("Time Step:", this));
  spnTimeStep_ = new QDoubleSpinBox(this);
  spnTimeStep_->setRange(0.1, 10.0);
  spnTimeStep_->setValue(2.0);
  spnTimeStep_->setSingleStep(0.1);
  spnTimeStep_->setSuffix(" s");
  spnTimeStep_->setMinimumWidth(100);
  row2Layout->addWidget(spnTimeStep_);

  row2Layout->addWidget(new QLabel("Speed:", this));
  cmbSpeed_ = new QComboBox(this);
  cmbSpeed_->addItem("1x (real-time)", 1);
  cmbSpeed_->addItem("2x (double speed)", 2);
  cmbSpeed_->addItem("5x", 5);
  cmbSpeed_->addItem("10x", 10);
  cmbSpeed_->addItem("20x", 20);
  cmbSpeed_->addItem("50x", 50);
  cmbSpeed_->addItem("100x", 100);
  cmbSpeed_->setCurrentIndex(0);
  cmbSpeed_->setMinimumWidth(130);
  cmbSpeed_->setToolTip("Simulation speed multiplier\n1x = real-time (1800 s takes 30 min)\n100x = fast (1800 s takes 18 s)");
  row2Layout->addWidget(cmbSpeed_);

  row2Layout->addWidget(new QLabel("Simulation:", this));
  cmbSimulationMode_ = new QComboBox(this);
  cmbSimulationMode_->addItem("Steady Clean (no fouling)");
  cmbSimulationMode_->addItem("Steady with Fouling");
  cmbSimulationMode_->addItem("Dynamic Clean (disturbances)");
  cmbSimulationMode_->addItem("Dynamic with Fouling");
  cmbSimulationMode_->setMinimumWidth(220);
  cmbSimulationMode_->setToolTip("Select preset behaviour for fouling and inlet disturbances.");
  cmbSimulationMode_->setCurrentIndex(static_cast<int>(simulationMode_));
  connect(cmbSimulationMode_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSimulationModeChanged);
  row2Layout->addWidget(cmbSimulationMode_);

  row2Layout->addWidget(new QLabel("Status:", this));
  lblStatus_ = new QLabel("Ready", this);
  lblStatus_->setStyleSheet("color: #27ae60; font-weight: bold; font-size: 10pt;");
  row2Layout->addWidget(lblStatus_);

  row2Layout->addStretch();

  return topBar;
}QWidget* MainWindow::createLeftPanel() {
  auto *panel = new QWidget(this);
  auto *layout = new QVBoxLayout(panel);
  layout->setSpacing(4);
  layout->setContentsMargins(5, 5, 5, 5);

  // === OPERATING CONDITIONS ===
  auto *grpOperating = new QGroupBox("Operating Conditions", this);
  auto *formOp = new QFormLayout(grpOperating);
  formOp->setSpacing(5);
  formOp->setContentsMargins(8, 8, 8, 8);
  formOp->setHorizontalSpacing(10);
  formOp->setVerticalSpacing(5);

  spnHotFlowRate_ = new QDoubleSpinBox(this);
  spnHotFlowRate_->setRange(0.1, 5.0);
  spnHotFlowRate_->setSingleStep(0.1);
  spnHotFlowRate_->setValue(op_.m_dot_hot);
  spnHotFlowRate_->setSuffix(" kg/s");
  connect(spnHotFlowRate_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Hot Flow Rate:", spnHotFlowRate_);

  spnColdFlowRate_ = new QDoubleSpinBox(this);
  spnColdFlowRate_->setRange(0.1, 5.0);
  spnColdFlowRate_->setSingleStep(0.1);
  spnColdFlowRate_->setValue(op_.m_dot_cold);
  spnColdFlowRate_->setSuffix(" kg/s");
  connect(spnColdFlowRate_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Cold Flow Rate:", spnColdFlowRate_);

  spnHotInletTemp_ = new QDoubleSpinBox(this);
  spnHotInletTemp_->setRange(40, 150);
  spnHotInletTemp_->setValue(op_.Tin_hot);
  spnHotInletTemp_->setSuffix(" \xC2\xB0" "C");
  connect(spnHotInletTemp_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Hot Inlet Temp:", spnHotInletTemp_);

  spnColdInletTemp_ = new QDoubleSpinBox(this);
  spnColdInletTemp_->setRange(5, 50);
  spnColdInletTemp_->setValue(op_.Tin_cold);
  spnColdInletTemp_->setSuffix(" \xC2\xB0" "C");
  connect(spnColdInletTemp_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Cold Inlet Temp:", spnColdInletTemp_);

  layout->addWidget(grpOperating);

  // === HOT FLUID PROPERTIES ===
  auto *grpHotFluid = new QGroupBox("Hot Fluid Properties", this);
  auto *formHot = new QFormLayout(grpHotFluid);
  formHot->setSpacing(5);
  formHot->setContentsMargins(8, 8, 8, 8);
  formHot->setHorizontalSpacing(10);
  formHot->setVerticalSpacing(5);

  cmbHotPreset_ = new QComboBox(this);
  for (const auto &info : hx::fluidPresetCatalog()) {
    cmbHotPreset_->addItem(info.displayName, static_cast<int>(info.preset));
  }
  cmbHotPreset_->setCurrentIndex(0);  // Custom
  cmbHotPreset_->setToolTip(
      "Select a fluid preset to use temperature-dependent ρ, μ, cp, k. "
      "Custom keeps the values entered below.");
  connect(cmbHotPreset_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int){
            auto preset = static_cast<hx::FluidPreset>(cmbHotPreset_->currentData().toInt());
            if (preset != hx::FluidPreset::Custom) {
              auto f = hx::evaluateFluid(preset, spnHotInletTemp_->value(), hot_);
              QSignalBlocker b1(spnHotDensity_); spnHotDensity_->setValue(f.rho);
              QSignalBlocker b2(spnHotViscosity_); spnHotViscosity_->setValue(f.mu);
              QSignalBlocker b3(spnHotSpecificHeat_); spnHotSpecificHeat_->setValue(f.cp);
              QSignalBlocker b4(spnHotConductivity_); spnHotConductivity_->setValue(f.k);
            }
            onParameterChanged();
          });
  formHot->addRow("Preset:", cmbHotPreset_);

  spnHotDensity_ = new QDoubleSpinBox(this);
  spnHotDensity_->setRange(500, 2000);
  spnHotDensity_->setValue(hot_.rho);
  spnHotDensity_->setSuffix(" kg/m\xC2\xB3");
  connect(spnHotDensity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Density (ρ):", spnHotDensity_);

  spnHotViscosity_ = new QDoubleSpinBox(this);
  spnHotViscosity_->setRange(0.0001, 0.01);
  spnHotViscosity_->setDecimals(5);
  spnHotViscosity_->setSingleStep(0.00001);
  spnHotViscosity_->setValue(hot_.mu);
  spnHotViscosity_->setSuffix(" Pa\xC2\xB7s");
  connect(spnHotViscosity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Viscosity (μ):", spnHotViscosity_);

  spnHotSpecificHeat_ = new QDoubleSpinBox(this);
  spnHotSpecificHeat_->setRange(1000, 10000);
  spnHotSpecificHeat_->setValue(hot_.cp);
  spnHotSpecificHeat_->setSuffix(" J/kg\xC2\xB7K");
  connect(spnHotSpecificHeat_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Specific Heat (cp):", spnHotSpecificHeat_);

  spnHotConductivity_ = new QDoubleSpinBox(this);
  spnHotConductivity_->setRange(0.01, 5.0);
  spnHotConductivity_->setDecimals(3);
  spnHotConductivity_->setValue(hot_.k);
  spnHotConductivity_->setSuffix(" W/m\xC2\xB7K");
  connect(spnHotConductivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Conductivity (k):", spnHotConductivity_);

  layout->addWidget(grpHotFluid);

  // === COLD FLUID PROPERTIES ===
  auto *grpColdFluid = new QGroupBox("Cold Fluid Properties", this);
  auto *formCold = new QFormLayout(grpColdFluid);
  formCold->setSpacing(5);
  formCold->setContentsMargins(8, 8, 8, 8);
  formCold->setHorizontalSpacing(10);
  formCold->setVerticalSpacing(5);

  cmbColdPreset_ = new QComboBox(this);
  for (const auto &info : hx::fluidPresetCatalog()) {
    cmbColdPreset_->addItem(info.displayName, static_cast<int>(info.preset));
  }
  cmbColdPreset_->setCurrentIndex(0);  // Custom
  cmbColdPreset_->setToolTip(
      "Select a fluid preset to use temperature-dependent ρ, μ, cp, k. "
      "Custom keeps the values entered below.");
  connect(cmbColdPreset_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int){
            auto preset = static_cast<hx::FluidPreset>(cmbColdPreset_->currentData().toInt());
            if (preset != hx::FluidPreset::Custom) {
              auto f = hx::evaluateFluid(preset, spnColdInletTemp_->value(), cold_);
              QSignalBlocker b1(spnColdDensity_); spnColdDensity_->setValue(f.rho);
              QSignalBlocker b2(spnColdViscosity_); spnColdViscosity_->setValue(f.mu);
              QSignalBlocker b3(spnColdSpecificHeat_); spnColdSpecificHeat_->setValue(f.cp);
              QSignalBlocker b4(spnColdConductivity_); spnColdConductivity_->setValue(f.k);
            }
            onParameterChanged();
          });
  formCold->addRow("Preset:", cmbColdPreset_);

  spnColdDensity_ = new QDoubleSpinBox(this);
  spnColdDensity_->setRange(500, 2000);
  spnColdDensity_->setValue(cold_.rho);
  spnColdDensity_->setSuffix(" kg/m\xC2\xB3");
  connect(spnColdDensity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Density (ρ):", spnColdDensity_);

  spnColdViscosity_ = new QDoubleSpinBox(this);
  spnColdViscosity_->setRange(0.0001, 0.01);
  spnColdViscosity_->setDecimals(5);
  spnColdViscosity_->setSingleStep(0.00001);
  spnColdViscosity_->setValue(cold_.mu);
  spnColdViscosity_->setSuffix(" Pa\xC2\xB7s");
  connect(spnColdViscosity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Viscosity (μ):", spnColdViscosity_);

  spnColdSpecificHeat_ = new QDoubleSpinBox(this);
  spnColdSpecificHeat_->setRange(1000, 10000);
  spnColdSpecificHeat_->setValue(cold_.cp);
  spnColdSpecificHeat_->setSuffix(" J/kg\xC2\xB7K");
  connect(spnColdSpecificHeat_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Specific Heat (cp):", spnColdSpecificHeat_);

  spnColdConductivity_ = new QDoubleSpinBox(this);
  spnColdConductivity_->setRange(0.01, 5.0);
  spnColdConductivity_->setDecimals(3);
  spnColdConductivity_->setValue(cold_.k);
  spnColdConductivity_->setSuffix(" W/m\xC2\xB7K");
  connect(spnColdConductivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Conductivity (k):", spnColdConductivity_);

  layout->addWidget(grpColdFluid);

  // === GEOMETRY ===
  auto *grpGeometry = new QGroupBox("Geometry", this);
  auto *formGeom = new QFormLayout(grpGeometry);
  formGeom->setSpacing(5);
  formGeom->setContentsMargins(8, 8, 8, 8);
  formGeom->setHorizontalSpacing(10);
  formGeom->setVerticalSpacing(5);

  spnNumTubes_ = new QSpinBox(this);
  spnNumTubes_->setRange(50, 500);
  spnNumTubes_->setValue(geom_.nTubes);
  connect(spnNumTubes_, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Number of Tubes:", spnNumTubes_);

  spnTubeDiameter_ = new QDoubleSpinBox(this);
  spnTubeDiameter_->setRange(10, 50);
  spnTubeDiameter_->setValue(geom_.Di * 1000);  // Convert to mm
  spnTubeDiameter_->setSuffix(" mm");
  connect(spnTubeDiameter_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Tube Inner Dia:", spnTubeDiameter_);

  spnTubeOuterDiameter_ = new QDoubleSpinBox(this);
  spnTubeOuterDiameter_->setRange(12, 60);
  spnTubeOuterDiameter_->setValue(geom_.Do * 1000);  // Convert to mm
  spnTubeOuterDiameter_->setSuffix(" mm");
  connect(spnTubeOuterDiameter_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Tube Outer Dia:", spnTubeOuterDiameter_);

  spnTubeLength_ = new QDoubleSpinBox(this);
  spnTubeLength_->setRange(1, 20);
  spnTubeLength_->setValue(geom_.L);
  spnTubeLength_->setSuffix(" m");
  connect(spnTubeLength_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Tube Length:", spnTubeLength_);

  spnTubePitch_ = new QDoubleSpinBox(this);
  spnTubePitch_->setRange(15, 100);
  spnTubePitch_->setValue(geom_.pitch * 1000);  // Convert to mm
  spnTubePitch_->setSuffix(" mm");
  connect(spnTubePitch_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Tube Pitch:", spnTubePitch_);

  spnShellDiameter_ = new QDoubleSpinBox(this);
  spnShellDiameter_->setRange(200, 1000);
  spnShellDiameter_->setValue(geom_.shellID * 1000);  // Convert to mm
  spnShellDiameter_->setSuffix(" mm");
  connect(spnShellDiameter_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Shell Inner Dia:", spnShellDiameter_);

  spnBaffleSpacing_ = new QDoubleSpinBox(this);
  spnBaffleSpacing_->setRange(100, 1000);
  spnBaffleSpacing_->setValue(geom_.baffleSpacing * 1000);  // Convert to mm
  spnBaffleSpacing_->setSuffix(" mm");
  connect(spnBaffleSpacing_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Baffle Spacing:", spnBaffleSpacing_);

  spnBaffleCutFraction_ = new QDoubleSpinBox(this);
  spnBaffleCutFraction_->setRange(0.15, 0.45);
  spnBaffleCutFraction_->setDecimals(2);
  spnBaffleCutFraction_->setSingleStep(0.05);
  spnBaffleCutFraction_->setValue(geom_.baffleCutFrac);
  connect(spnBaffleCutFraction_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Baffle Cut Fraction:", spnBaffleCutFraction_);

  spnNumBaffles_ = new QSpinBox(this);
  spnNumBaffles_->setRange(5, 50);
  spnNumBaffles_->setValue(geom_.nBaffles);
  connect(spnNumBaffles_, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Number of Baffles:", spnNumBaffles_);

  spnWallConductivity_ = new QDoubleSpinBox(this);
  spnWallConductivity_->setRange(10, 500);
  spnWallConductivity_->setValue(geom_.wall_k);
  spnWallConductivity_->setSuffix(" W/m\xC2\xB7K");
  connect(spnWallConductivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Wall Conductivity:", spnWallConductivity_);

  spnWallThickness_ = new QDoubleSpinBox(this);
  spnWallThickness_->setRange(0.5, 5.0);
  spnWallThickness_->setDecimals(2);
  spnWallThickness_->setValue(geom_.wall_thickness * 1000);  // Convert to mm
  spnWallThickness_->setSuffix(" mm");
  connect(spnWallThickness_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Wall Thickness:", spnWallThickness_);

  cmbFlowArrangement_ = new QComboBox(this);
  cmbFlowArrangement_->addItem("Counter-flow (idealised)",   static_cast<int>(hx::FlowArrangement::CounterFlow));
  cmbFlowArrangement_->addItem("Parallel-flow (co-current)", static_cast<int>(hx::FlowArrangement::ParallelFlow));
  cmbFlowArrangement_->addItem("Shell & Tube 1-2 (TEMA E)",  static_cast<int>(hx::FlowArrangement::ShellTube_1_2));
  cmbFlowArrangement_->addItem("Shell & Tube 2-4 (TEMA F)",  static_cast<int>(hx::FlowArrangement::ShellTube_2_4));
  cmbFlowArrangement_->setCurrentIndex(0);
  cmbFlowArrangement_->setToolTip(
      "Flow arrangement determines the ε–NTU formula. Shell-&-tube "
      "configurations apply the Bowman LMTD correction factor F.");
  connect(cmbFlowArrangement_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onParameterChanged);
  formGeom->addRow("Flow Arrangement:", cmbFlowArrangement_);

  layout->addWidget(grpGeometry);

  // === FOULING ===
  grpFouling_ = new QGroupBox("Fouling Model", this);
  auto *formFoul = new QFormLayout(grpFouling_);
  formFoul->setSpacing(5);
  formFoul->setContentsMargins(8, 8, 8, 8);
  formFoul->setHorizontalSpacing(10);
  formFoul->setVerticalSpacing(5);

  spnRf0_ = new QDoubleSpinBox(this);
  spnRf0_->setRange(0.0, 0.01);
  spnRf0_->setDecimals(6);
  spnRf0_->setSingleStep(0.00001);
  spnRf0_->setValue(foulParams_.Rf0);
  spnRf0_->setSuffix(" m\xC2\xB2\xC2\xB7K/W");
  connect(spnRf0_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Initial Rf (Rf0):", spnRf0_);

  spnRfMax_ = new QDoubleSpinBox(this);
  spnRfMax_->setRange(0.0, 0.01);
  spnRfMax_->setDecimals(5);
  spnRfMax_->setSingleStep(0.0001);
  spnRfMax_->setValue(foulParams_.RfMax);
  spnRfMax_->setSuffix(" m\xC2\xB2\xC2\xB7K/W");
  connect(spnRfMax_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Max Rf (RfMax):", spnRfMax_);

  spnTau_ = new QDoubleSpinBox(this);
  spnTau_->setRange(300, 10000);
  spnTau_->setValue(foulParams_.tau);
  spnTau_->setSuffix(" s");
  connect(spnTau_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Time Constant (τ):", spnTau_);

  spnAlpha_ = new QDoubleSpinBox(this);
  spnAlpha_->setRange(0.0, 0.001);
  spnAlpha_->setDecimals(7);
  spnAlpha_->setSingleStep(0.0000001);
  spnAlpha_->setValue(foulParams_.alpha);
  spnAlpha_->setSuffix(" m\xC2\xB2\xC2\xB7K/(W\xC2\xB7s)");
  connect(spnAlpha_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Linear Rate (α):", spnAlpha_);

  cmbFoulingModel_ = new QComboBox(this);
  cmbFoulingModel_->addItem("Asymptotic");
  cmbFoulingModel_->addItem("Linear");
  cmbFoulingModel_->setCurrentIndex(
    foulParams_.model == hx::FoulingParams::Model::Asymptotic ? 0 : 1);
  connect(cmbFoulingModel_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Fouling Model:", cmbFoulingModel_);

  layout->addWidget(grpFouling_);

  // === PID CONTROLLER ===
  grpPid_ = new QGroupBox("PID Control (cold flow → Tc,out setpoint)", this);
  grpPid_->setCheckable(true);
  grpPid_->setChecked(false);
  connect(grpPid_, &QGroupBox::toggled, this, &MainWindow::onParameterChanged);
  auto *formPid = new QFormLayout(grpPid_);
  formPid->setSpacing(5);
  formPid->setContentsMargins(8, 8, 8, 8);
  formPid->setHorizontalSpacing(10);
  formPid->setVerticalSpacing(5);

  spnPidSetpoint_ = new QDoubleSpinBox(this);
  spnPidSetpoint_->setRange(5.0, 150.0);
  spnPidSetpoint_->setDecimals(1);
  spnPidSetpoint_->setSingleStep(0.5);
  spnPidSetpoint_->setValue(45.0);
  spnPidSetpoint_->setSuffix(" \xC2\xB0""C");
  connect(spnPidSetpoint_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("Setpoint T\xE2\x82\x9C,\xE2\x82\x92\xE1\xB5\xA4\xE1\xB5\x97:", spnPidSetpoint_);

  spnPidKp_ = new QDoubleSpinBox(this);
  spnPidKp_->setRange(0.0, 5.0);
  spnPidKp_->setDecimals(4);
  spnPidKp_->setSingleStep(0.01);
  spnPidKp_->setValue(0.05);
  spnPidKp_->setSuffix(" kg/s/\xC2\xB0""C");
  connect(spnPidKp_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("K\xE1\xB5\x96 (proportional):", spnPidKp_);

  spnPidKi_ = new QDoubleSpinBox(this);
  spnPidKi_->setRange(0.0, 1.0);
  spnPidKi_->setDecimals(5);
  spnPidKi_->setSingleStep(0.001);
  spnPidKi_->setValue(0.005);
  spnPidKi_->setSuffix(" kg/s/\xC2\xB0""C/s");
  connect(spnPidKi_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("K\xE1\xB5\xA2 (integral):", spnPidKi_);

  spnPidKd_ = new QDoubleSpinBox(this);
  spnPidKd_->setRange(0.0, 5.0);
  spnPidKd_->setDecimals(4);
  spnPidKd_->setSingleStep(0.01);
  spnPidKd_->setValue(0.0);
  spnPidKd_->setSuffix(" kg\xC2\xB7s/\xC2\xB0""C");
  connect(spnPidKd_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("K\xE1\xB5\x88 (derivative):", spnPidKd_);

  spnPidUMin_ = new QDoubleSpinBox(this);
  spnPidUMin_->setRange(0.01, 10.0);
  spnPidUMin_->setDecimals(2);
  spnPidUMin_->setSingleStep(0.05);
  spnPidUMin_->setValue(0.1);
  spnPidUMin_->setSuffix(" kg/s");
  connect(spnPidUMin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("u min:", spnPidUMin_);

  spnPidUMax_ = new QDoubleSpinBox(this);
  spnPidUMax_->setRange(0.1, 20.0);
  spnPidUMax_->setDecimals(2);
  spnPidUMax_->setSingleStep(0.1);
  spnPidUMax_->setValue(5.0);
  spnPidUMax_->setSuffix(" kg/s");
  connect(spnPidUMax_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formPid->addRow("u max:", spnPidUMax_);

  layout->addWidget(grpPid_);

  // === LIMITS ===
  auto *grpLimits = new QGroupBox("Operating Limits", this);
  auto *formLimits = new QFormLayout(grpLimits);
  formLimits->setSpacing(5);
  formLimits->setContentsMargins(8, 8, 8, 8);
  formLimits->setHorizontalSpacing(10);
  formLimits->setVerticalSpacing(5);

  spnMaxTubePressureDrop_ = new QDoubleSpinBox(this);
  spnMaxTubePressureDrop_->setRange(1000, 100000);
  spnMaxTubePressureDrop_->setValue(50000);  // Default
  spnMaxTubePressureDrop_->setSuffix(" Pa");
  connect(spnMaxTubePressureDrop_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formLimits->addRow("Max Tube ΔP:", spnMaxTubePressureDrop_);

  spnMaxShellPressureDrop_ = new QDoubleSpinBox(this);
  spnMaxShellPressureDrop_->setRange(1000, 100000);
  spnMaxShellPressureDrop_->setValue(30000);  // Default
  spnMaxShellPressureDrop_->setSuffix(" Pa");
  connect(spnMaxShellPressureDrop_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formLimits->addRow("Max Shell ΔP:", spnMaxShellPressureDrop_);

  spnMinColdFlowRate_ = new QDoubleSpinBox(this);
  spnMinColdFlowRate_->setRange(0.01, 5.0);
  spnMinColdFlowRate_->setSingleStep(0.1);
  spnMinColdFlowRate_->setValue(0.5);  // Default
  spnMinColdFlowRate_->setSuffix(" kg/s");
  connect(spnMinColdFlowRate_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formLimits->addRow("Min Cold Flow:", spnMinColdFlowRate_);

  spnMaxColdFlowRate_ = new QDoubleSpinBox(this);
  spnMaxColdFlowRate_->setRange(1.0, 10.0);
  spnMaxColdFlowRate_->setSingleStep(0.1);
  spnMaxColdFlowRate_->setValue(5.0);  // Default
  spnMaxColdFlowRate_->setSuffix(" kg/s");
  connect(spnMaxColdFlowRate_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formLimits->addRow("Max Cold Flow:", spnMaxColdFlowRate_);

  layout->addWidget(grpLimits);

  layout->addStretch();

  // Wrap in scroll area since we have many parameters now
  auto *scrollArea = new QScrollArea(this);
  scrollArea->setWidget(panel);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setMinimumWidth(480);
  scrollArea->setMaximumWidth(600);
  
  // Ensure scroll area background matches
  scrollArea->setStyleSheet("QScrollArea { background: #f5f5f5; border: none; }");

  return scrollArea;
}

void MainWindow::createChartTabs() {
  chartTabs_ = new QTabWidget(this);
  chartTabs_->setDocumentMode(true);
  chartTabs_->setTabPosition(QTabWidget::North);

  // Create 4 specialized charts
  chartTemp_ = new ChartWidget(ChartWidget::TEMPERATURE, this);
  chartHeat_ = new ChartWidget(ChartWidget::HEAT_DUTY, this);
  chartPressure_ = new ChartWidget(ChartWidget::PRESSURE, this);
  chartFouling_ = new ChartWidget(ChartWidget::FOULING, this);
  chartPID_ = new ChartWidget(ChartWidget::PID_CONTROL, this);
  exchWidget_ = new HeatExchangerWidget(this);
  exchWidget_->setGeometryData(geom_);
  exchWidget_->setOperatingPoint(op_);
  kpiPanel_ = new KPIPanel(this);

  // Connect flow control signals from the visualization widget back to spinboxes
  connect(exchWidget_, &HeatExchangerWidget::hotFlowRateChanged, this, [this](double val) {
    QSignalBlocker b(spnHotFlowRate_);
    spnHotFlowRate_->setValue(val);
    onParameterChanged();
  });
  connect(exchWidget_, &HeatExchangerWidget::coldFlowRateChanged, this, [this](double val) {
    QSignalBlocker b(spnColdFlowRate_);
    spnColdFlowRate_->setValue(val);
    onParameterChanged();
  });

  // Add to tabs
  chartTabs_->addTab(chartTemp_, "Temperatures");
  chartTabs_->addTab(chartHeat_, "Heat Duty & U");
  chartTabs_->addTab(chartPressure_, "Pressure Drops");
  chartTabs_->addTab(chartFouling_, "Fouling");
  chartTabs_->addTab(chartPID_, "PID Control");
  chartTabs_->addTab(exchWidget_, "Heat Exchanger View");
  chartTabs_->addTab(kpiPanel_, "KPI Dashboard");
}

void MainWindow::applyModernStyle() {
  setStyleSheet(R"(
    QMainWindow {
      background-color: #f5f5f5;
    }
    
    /* Fix for all labels - make text visible */
    QLabel {
      color: #2c3e50;
      font-size: 10pt;
    }
    
    /* Fix for form labels specifically */
    QFormLayout QLabel {
      color: #34495e;
      font-size: 9pt;
      font-weight: 600;
      padding-right: 5px;
    }
    
    QGroupBox {
      border: 2px solid #3498db;
      border-radius: 5px;
      margin-top: 6px;
      margin-bottom: 3px;
      font-weight: bold;
      font-size: 9pt;
      padding: 5px;
      padding-top: 18px;
      background: white;
      color: #2c3e50;
    }
    
    QGroupBox::title {
      subcontrol-origin: margin;
      subcontrol-position: top left;
      left: 10px;
      top: 2px;
      padding: 2px 8px;
      background: #3498db;
      color: white;
      border-radius: 3px;
      font-weight: bold;
      font-size: 8pt;
    }
    
    /* SpinBox styling with visible text */
    QDoubleSpinBox, QSpinBox {
      border: 1px solid #bdc3c7;
      border-radius: 3px;
      padding: 3px 6px;
      background: white;
      color: #2c3e50;
      min-width: 120px;
      min-height: 22px;
      max-height: 26px;
      font-size: 9pt;
      selection-background-color: #3498db;
      selection-color: white;
      font-weight: normal;
    }
    
    QDoubleSpinBox:focus, QSpinBox:focus {
      border: 2px solid #3498db;
      background: white;
    }
    
    QDoubleSpinBox:disabled, QSpinBox:disabled {
      background: #ecf0f1;
      color: #7f8c8d;
    }
    
    /* Ensure spinbox buttons are visible */
    QDoubleSpinBox::up-button, QSpinBox::up-button,
    QDoubleSpinBox::down-button, QSpinBox::down-button {
      background: #d5e8f7;
      border: 1px solid #bdc3c7;
      width: 18px;
    }
    
    QDoubleSpinBox::up-button:hover, QSpinBox::up-button:hover,
    QDoubleSpinBox::down-button:hover, QSpinBox::down-button:hover {
      background: #3498db;
    }
    
    /* ComboBox styling */
    QComboBox {
      border: 1px solid #bdc3c7;
      border-radius: 3px;
      padding: 3px 6px;
      background: white;
      color: #2c3e50;
      min-width: 120px;
      min-height: 22px;
      max-height: 26px;
      font-size: 9pt;
      font-weight: normal;
    }
    
    QComboBox:focus {
      border: 2px solid #3498db;
    }
    
    QComboBox::drop-down {
      border: none;
      background: #d5e8f7;
      width: 20px;
    }
    
    QComboBox::down-arrow {
      image: none;
      border: 2px solid #2c3e50;
      width: 6px;
      height: 6px;
      border-top: none;
      border-left: none;
      margin-right: 5px;
    }
    
    QComboBox QAbstractItemView {
      background: white;
      color: #2c3e50;
      selection-background-color: #3498db;
      selection-color: white;
      border: 1px solid #bdc3c7;
    }
    
    QPushButton {
      border-radius: 4px;
      padding: 8px 14px;
      font-weight: bold;
      font-size: 10pt;
      min-height: 38px;
      max-height: 44px;
      color: #2c3e50;
      background: #ecf0f1;
      border: 1px solid #bdc3c7;
    }
    
    QPushButton:hover {
      background: #d5dbdb;
    }
    
    QPushButton#startButton {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                  stop:0 #2ecc71, stop:1 #27ae60);
      color: white;
      border: none;
    }
    
    QPushButton#startButton:hover {
      background: #27ae60;
    }
    
    QPushButton#stopButton {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                  stop:0 #e74c3c, stop:1 #c0392b);
      color: white;
      border: none;
    }
    
    QPushButton#stopButton:hover {
      background: #c0392b;
    }
    
    QPushButton#pauseButton {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                  stop:0 #f39c12, stop:1 #e67e22);
      color: white;
      border: none;
    }
    
    QPushButton#pauseButton:hover {
      background: #e67e22;
    }
    
    QPushButton:disabled {
      background: #bdc3c7;
      color: #7f8c8d;
    }
    
    QFrame {
      background: #bdc3c7;
    }
    
    QTabWidget::pane {
      border: 2px solid #ecf0f1;
      border-radius: 4px;
      background: white;
    }
    
    QTabBar::tab {
      background: #ecf0f1;
      border: 1px solid #bdc3c7;
      padding: 8px 16px;
      margin-right: 2px;
      border-top-left-radius: 4px;
      border-top-right-radius: 4px;
      font-weight: bold;
      color: #2c3e50;
    }
    
    QTabBar::tab:selected {
      background: white;
      border-bottom-color: white;
      color: #2c3e50;
    }
    
    QTabBar::tab:hover {
      background: #d5dbdb;
      color: #2c3e50;
    }
    
    QStatusBar {
      background: #34495e;
      color: white;
      font-weight: bold;
    }
    
    /* Widget container styling */
    QWidget {
      color: #2c3e50;
    }
  )");
}

void MainWindow::updateSimulationCore() {
  // Update parameters from UI - OPERATING CONDITIONS
  op_.m_dot_hot = spnHotFlowRate_->value();
  op_.m_dot_cold = spnColdFlowRate_->value();
  op_.Tin_hot = spnHotInletTemp_->value();
  op_.Tin_cold = spnColdInletTemp_->value();

  // HOT FLUID PROPERTIES
  hot_.rho = spnHotDensity_->value();
  hot_.mu = spnHotViscosity_->value();
  hot_.cp = spnHotSpecificHeat_->value();
  hot_.k = spnHotConductivity_->value();

  // COLD FLUID PROPERTIES
  cold_.rho = spnColdDensity_->value();
  cold_.mu = spnColdViscosity_->value();
  cold_.cp = spnColdSpecificHeat_->value();
  cold_.k = spnColdConductivity_->value();

  // GEOMETRY
  geom_.nTubes = spnNumTubes_->value();
  geom_.Di = spnTubeDiameter_->value() / 1000.0;  // mm to m
  geom_.Do = spnTubeOuterDiameter_->value() / 1000.0;  // mm to m
  geom_.L = spnTubeLength_->value();
  geom_.pitch = spnTubePitch_->value() / 1000.0;  // mm to m
  geom_.shellID = spnShellDiameter_->value() / 1000.0;  // mm to m
  geom_.baffleSpacing = spnBaffleSpacing_->value() / 1000.0;  // mm to m
  geom_.baffleCutFrac = spnBaffleCutFraction_->value();
  geom_.nBaffles = spnNumBaffles_->value();
  geom_.wall_k = spnWallConductivity_->value();
  geom_.wall_thickness = spnWallThickness_->value() / 1000.0;  // mm to m

  // FOULING
  foulParams_.Rf0 = spnRf0_->value();
  foulParams_.RfMax = spnRfMax_->value();
  foulParams_.tau = spnTau_->value();
  foulParams_.alpha = spnAlpha_->value();
  foulParams_.model = (cmbFoulingModel_->currentIndex() == 0) 
                       ? hx::FoulingParams::Model::Asymptotic 
                       : hx::FoulingParams::Model::Linear;

  // SIMULATION CONFIG
  simConfig_.dt = spnTimeStep_->value();
  simConfig_.tEnd = spnDuration_->value();

  // OPERATING LIMITS (used by KPI margin calculations and validators)
  simConfig_.limits.dP_tube_max     = spnMaxTubePressureDrop_->value();
  simConfig_.limits.dP_shell_max    = spnMaxShellPressureDrop_->value();
  simConfig_.limits.m_dot_cold_min  = spnMinColdFlowRate_->value();
  simConfig_.limits.m_dot_cold_max  = spnMaxColdFlowRate_->value();

  // FLOW ARRANGEMENT (ε–NTU / LMTD-F factor)
  simConfig_.arrangement = cmbFlowArrangement_
      ? static_cast<hx::FlowArrangement>(cmbFlowArrangement_->currentData().toInt())
      : hx::FlowArrangement::CounterFlow;

  // FLUID PRESETS (feeds T-dependent property library)
  simConfig_.hotPreset  = cmbHotPreset_
      ? static_cast<hx::FluidPreset>(cmbHotPreset_->currentData().toInt())
      : hx::FluidPreset::Custom;
  simConfig_.coldPreset = cmbColdPreset_
      ? static_cast<hx::FluidPreset>(cmbColdPreset_->currentData().toInt())
      : hx::FluidPreset::Custom;
  simConfig_.hotCustom  = hot_;
  simConfig_.coldCustom = cold_;

  // PID CONTROLLER
  simConfig_.pid.enabled        = grpPid_ ? grpPid_->isChecked() : false;
  simConfig_.pid.setpoint_Tc_out = spnPidSetpoint_ ? spnPidSetpoint_->value() : 45.0;
  simConfig_.pid.kp             = spnPidKp_ ? spnPidKp_->value() : 0.05;
  simConfig_.pid.ki             = spnPidKi_ ? spnPidKi_->value() : 0.005;
  simConfig_.pid.kd             = spnPidKd_ ? spnPidKd_->value() : 0.0;
  simConfig_.pid.u_min          = spnPidUMin_ ? spnPidUMin_->value() : 0.1;
  simConfig_.pid.u_max          = spnPidUMax_ ? spnPidUMax_->value() : 5.0;

  // Recreate simulation objects with updated parameters
  thermo_ = std::make_unique<hx::Thermo>(geom_, hot_, cold_);
  hydro_ = std::make_unique<hx::Hydraulics>(geom_, hot_, cold_);
  fouling_ = std::make_unique<hx::Fouling>(foulParams_);

  if (exchWidget_) {
    exchWidget_->setGeometryData(geom_);
    exchWidget_->setOperatingPoint(op_);
  }
}

void MainWindow::onParameterChanged() {
  if (!isRunning_) {
    // Restart debounce timer to delay geometry/simulation updates
    // This prevents excessive updates during rapid spinbox changes
    if (geometryUpdateTimer_) {
      geometryUpdateTimer_->stop();
      geometryUpdateTimer_->start();
    } else {
      updateSimulationCore();
    }
  }
}

void MainWindow::onStart() {
  updateSimulationCore();

  // Clear all charts and data
  chartTemp_->clear();
  chartHeat_->clear();
  chartPressure_->clear();
  chartFouling_->clear();
  if (chartPID_) chartPID_->clear();
  if (kpiPanel_) kpiPanel_->reset();
  U_clean_baseline_ = 0.0;
  simulationData_.clear();

  // Create simulator
  auto simulator = std::make_unique<hx::Simulator>(*thermo_, *hydro_, *fouling_, simConfig_);
  simulator->reset(op_);
  
  // Apply simulation mode
  simulationMode_ = static_cast<SimulationMode>(std::clamp(cmbSimulationMode_->currentIndex(), 0, 3));
  applySimulationModeToUi();
  const ModeConfig cfg = modeConfig(simulationMode_);
  simulator->setSteadyStateMode(cfg.steady);
  simulator->setFoulingEnabled(cfg.fouling);

  // Create worker and thread
  simThread_ = new QThread(this);
  simWorker_ = new SimWorker();
  simWorker_->setSimulator(std::move(simulator), simConfig_);
  
  // Set speed multiplier from combo box
  int speedMultiplier = cmbSpeed_->currentData().toInt();
  simWorker_->setSpeedMultiplier(speedMultiplier);
  
  
  simWorker_->moveToThread(simThread_);

  // Connect signals
  connect(simThread_, &QThread::started, simWorker_, &SimWorker::run);
  connect(simWorker_, &SimWorker::sampleReady, this, &MainWindow::onSimulationSample);
  connect(simWorker_, &SimWorker::finished, this, &MainWindow::onSimulationFinished);
  connect(simWorker_, &SimWorker::finished, simThread_, &QThread::quit);
  connect(simThread_, &QThread::finished, simWorker_, &QObject::deleteLater);

  // UI state
  btnStart_->setEnabled(false);
  btnPause_->setEnabled(true);
  btnStop_->setEnabled(true);
  btnExport_->setEnabled(false);
  btnSnapshot_->setEnabled(false);
  btnReport_->setEnabled(false);
  isPaused_ = false;
  lblStatus_->setText("Running");
  lblStatus_->setStyleSheet("color: #e67e22; font-weight: bold; font-size: 10pt;");
  isRunning_ = true;

  // Start
  if (exchWidget_) exchWidget_->setSimulationRunning(true);
  simThread_->start();
  statusBar()->showMessage("Simulation running...");
}

void MainWindow::onStop() {
  if (simWorker_) {
    simWorker_->stop();
  }
  if (exchWidget_) exchWidget_->setSimulationRunning(false);

  // Update UI immediately
  btnStart_->setEnabled(true);
  btnPause_->setEnabled(false);
  btnStop_->setEnabled(false);
  isPaused_ = false;

  statusBar()->showMessage("Stopping simulation...");
}

void MainWindow::onSimulationSample(double t, const hx::State& state) {
  // Store data for export
  simulationData_.push_back({t, state});
  
  // Add to all charts
  chartTemp_->addSample(t, state);
  chartHeat_->addSample(t, state);
  chartPressure_->addSample(t, state);
  chartFouling_->addSample(t, state);
  if (chartPID_) {
    chartPID_->addSample(t, state, state.pidSetpoint, state.pidColdFlow);
  }
  if (exchWidget_) {
    exchWidget_->updateSimulationState(state);
  }
  if (kpiPanel_) {
    // Capture clean-U baseline once, at first valid sample of the run.
    if (U_clean_baseline_ <= 0.0 && state.U > 0.0 && state.Rf <= 1e-9) {
      U_clean_baseline_ = state.U;
    }
    const double baseline = (U_clean_baseline_ > 0.0) ? U_clean_baseline_ : state.U;
    kpiPanel_->update(state, op_, hot_, cold_, geom_, simConfig_.limits, baseline);
  }
}

void MainWindow::onSimulationFinished() {
  btnStart_->setEnabled(true);
  btnPause_->setEnabled(false);
  btnStop_->setEnabled(false);
  btnExport_->setEnabled(true);
  const bool haveData = !simulationData_.empty();
  btnSnapshot_->setEnabled(haveData);
  btnReport_->setEnabled(haveData);
  if (exchWidget_) exchWidget_->setSimulationRunning(false);
  lblStatus_->setText("Complete");
  lblStatus_->setStyleSheet("color: #27ae60; font-weight: bold; font-size: 10pt;");
  isRunning_ = false;
  isPaused_ = false;
  statusBar()->showMessage(QString("Simulation completed successfully - %1 data points collected")
                          .arg(simulationData_.size()), 5000);
}

void MainWindow::onReset() {
  if (isRunning_) {
    QMessageBox::warning(this, "Simulation Running",
                        "Please stop the simulation before resetting.");
    return;
  }

  resetToDefaults();

  // Update UI inputs to defaults
  spnHotFlowRate_->setValue(op_.m_dot_hot);
  spnColdFlowRate_->setValue(op_.m_dot_cold);
  spnHotInletTemp_->setValue(op_.Tin_hot);
  spnColdInletTemp_->setValue(op_.Tin_cold);
  spnNumTubes_->setValue(geom_.nTubes);
  spnTubeDiameter_->setValue(geom_.Di * 1000);
  spnTubeLength_->setValue(geom_.L);
  spnShellDiameter_->setValue(geom_.shellID * 1000);
  spnBaffleSpacing_->setValue(geom_.baffleSpacing * 1000);
  spnNumBaffles_->setValue(geom_.nBaffles);
  spnRfMax_->setValue(foulParams_.RfMax);
  spnTau_->setValue(foulParams_.tau);
  spnDuration_->setValue(simConfig_.tEnd);
  spnTimeStep_->setValue(simConfig_.dt);

  applySimulationModeToUi();
  updateSimulationCore();

  // Clear charts
  chartTemp_->clear();
  chartHeat_->clear();
  chartPressure_->clear();
  chartFouling_->clear();
  if (chartPID_) chartPID_->clear();
  if (kpiPanel_) kpiPanel_->reset();
  U_clean_baseline_ = 0.0;
  simulationData_.clear();
  btnExport_->setEnabled(false);
  btnSnapshot_->setEnabled(false);
  btnReport_->setEnabled(false);

  statusBar()->showMessage("Reset to default values", 3000);
}

void MainWindow::onPause() {
  if (!simWorker_) return;
  
  isPaused_ = !isPaused_;
  
  if (isPaused_) {
    // Pause simulation
    simWorker_->stop();  // Stop temporarily
    btnPause_->setText("Resume");
    btnPause_->setStyleSheet("background: #27ae60; color: white;");
    lblStatus_->setText("Paused");
    lblStatus_->setStyleSheet("color: #f39c12; font-weight: bold; font-size: 10pt;");
    statusBar()->showMessage("Simulation paused");
  } else {
    // Resume simulation
    btnPause_->setText("Pause");
    btnPause_->setStyleSheet("");  // Reset to default
    lblStatus_->setText("Running");
    lblStatus_->setStyleSheet("color: #e67e22; font-weight: bold; font-size: 10pt;");
    statusBar()->showMessage("Simulation resumed");
    // Note: Full resume would require more complex state management
    QMessageBox::information(this, "Resume", "Please use STOP and START to continue simulation.");
  }
}

void MainWindow::onZoomIn() {
  int currentTab = chartTabs_->currentIndex();
  switch (currentTab) {
    case 0: chartTemp_->zoomIn(); break;
    case 1: chartHeat_->zoomIn(); break;
    case 2: chartPressure_->zoomIn(); break;
    case 3: chartFouling_->zoomIn(); break;
    case 4:
      if (exchWidget_) exchWidget_->zoomIn();
      break;
  }
}

void MainWindow::onZoomOut() {
  int currentTab = chartTabs_->currentIndex();
  switch (currentTab) {
    case 0: chartTemp_->zoomOut(); break;
    case 1: chartHeat_->zoomOut(); break;
    case 2: chartPressure_->zoomOut(); break;
    case 3: chartFouling_->zoomOut(); break;
    case 4:
      if (exchWidget_) exchWidget_->zoomOut();
      break;
  }
}

void MainWindow::onZoomReset() {
  int currentTab = chartTabs_->currentIndex();
  switch (currentTab) {
    case 0: chartTemp_->resetZoom(); break;
    case 1: chartHeat_->resetZoom(); break;
    case 2: chartPressure_->resetZoom(); break;
    case 3: chartFouling_->resetZoom(); break;
    case 4:
      if (exchWidget_) exchWidget_->resetZoom();
      break;
  }
}

void MainWindow::onExportData() {
  QString filename = QFileDialog::getSaveFileName(this, "Export Simulation Data",
                                                   "simulation_data.csv", "CSV Files (*.csv)");
  if (filename.isEmpty()) return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open file for writing!");
    return;
  }

  QTextStream out(&file);
  
  // Write header
  out << "Time[s],Tc_out[C],Th_out[C],Q[W],U[W/m2K],Rf[m2K/W],dP_tube[Pa],dP_shell[Pa]\n";
  
  // Write data
  for (const auto& [t, state] : simulationData_) {
    out << t << "," 
        << state.Tc_out << "," 
        << state.Th_out << ","
        << state.Q << "," 
        << state.U << "," 
        << state.Rf << ","
        << state.dP_tube << "," 
        << state.dP_shell << "\n";
  }
  
  file.close();
  statusBar()->showMessage(QString("Data exported to %1").arg(filename), 5000);
  QMessageBox::information(this, "Export Complete",
                          QString("Simulation data exported to:\n%1\n\n%2 data points saved.")
                          .arg(filename).arg(simulationData_.size()));
}

void MainWindow::onSnapshotBaseline() {
  if (simulationData_.empty()) {
    statusBar()->showMessage("Run a simulation first before capturing a baseline", 4000);
    return;
  }
  chartTemp_->captureBaseline();
  chartHeat_->captureBaseline();
  chartPressure_->captureBaseline();
  chartFouling_->captureBaseline();
  if (chartPID_) chartPID_->captureBaseline();
  btnClearBaseline_->setEnabled(true);
  statusBar()->showMessage(
      "Baseline captured - next run overlays dashed reference traces on all charts", 5000);
}

void MainWindow::onClearBaseline() {
  chartTemp_->clearBaseline();
  chartHeat_->clearBaseline();
  chartPressure_->clearBaseline();
  chartFouling_->clearBaseline();
  if (chartPID_) chartPID_->clearBaseline();
  btnClearBaseline_->setEnabled(false);
  statusBar()->showMessage("Baseline overlay cleared", 3000);
}

namespace {

QString fluidPresetLabel(hx::FluidPreset p) {
  switch (p) {
    case hx::FluidPreset::Water:             return QStringLiteral("Water (IAPWS-IF97 fit)");
    case hx::FluidPreset::EthyleneGlycol30:  return QStringLiteral("Ethylene Glycol 30%");
    case hx::FluidPreset::EthyleneGlycol50:  return QStringLiteral("Ethylene Glycol 50%");
    case hx::FluidPreset::EngineOilSAE30:    return QStringLiteral("Engine Oil SAE30");
    case hx::FluidPreset::AirSTP:            return QStringLiteral("Air (Sutherland)");
    case hx::FluidPreset::Custom:
    default:                                 return QStringLiteral("Custom (user-specified)");
  }
}

QString arrangementLabel(hx::FlowArrangement a) {
  switch (a) {
    case hx::FlowArrangement::CounterFlow:   return QStringLiteral("Counter-flow (pure)");
    case hx::FlowArrangement::ParallelFlow:  return QStringLiteral("Parallel-flow (co-current)");
    case hx::FlowArrangement::ShellTube_1_2: return QStringLiteral("Shell-&-Tube 1-2 (TEMA, Bowman F)");
    case hx::FlowArrangement::ShellTube_2_4: return QStringLiteral("Shell-&-Tube 2-4 (TEMA, Bowman F)");
  }
  return QStringLiteral("Unknown");
}

QString htmlRow(const QString &label, const QString &value, const QString &unit = QString()) {
  const QString unitHtml = unit.isEmpty() ? QString() : QStringLiteral(" <span class=\"u\">%1</span>").arg(unit);
  return QStringLiteral("<tr><th>%1</th><td>%2%3</td></tr>").arg(label, value, unitHtml);
}

QString fmt(double v, int prec = 3) {
  if (!std::isfinite(v)) return QStringLiteral("—");
  return QLocale::c().toString(v, 'g', prec);
}

} // namespace

void MainWindow::onGenerateReport() {
  if (simulationData_.empty()) {
    QMessageBox::information(this, "No Data",
        "Run a simulation first - the report needs final-state values to summarise.");
    return;
  }

  const QString suggested = QStringLiteral("HeatXTwin_Report_%1.html")
      .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
  const QString filename = QFileDialog::getSaveFileName(
      this, "Save Simulation Report", suggested, "HTML Files (*.html)");
  if (filename.isEmpty()) return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open file for writing.");
    return;
  }

  const auto &[t_final, s_final] = simulationData_.back();
  const double dT_hot  = op_.Tin_hot  - s_final.Th_out;
  const double dT_cold = s_final.Tc_out - op_.Tin_cold;
  const double Ch = op_.m_dot_hot  * hot_.cp;
  const double Cc = op_.m_dot_cold * cold_.cp;
  const double Cmin = std::min(Ch, Cc);
  const double Cmax = std::max(Ch, Cc);
  const double Cr   = (Cmax > 0.0) ? Cmin / Cmax : 0.0;
  const double Qmax = (Cmin > 0.0) ? Cmin * (op_.Tin_hot - op_.Tin_cold) : 0.0;
  const double eps  = (Qmax > 1e-9) ? s_final.Q / Qmax : 0.0;
  const double NTU  = (Cmin > 0.0 && s_final.U > 0.0)
                        ? (s_final.U * geom_.areaOuter()) / Cmin : 0.0;
  const double U_clean = (U_clean_baseline_ > 0.0) ? U_clean_baseline_ : s_final.U;
  const double foulingPenalty = (U_clean > 0.0) ? 100.0 * (1.0 - s_final.U / U_clean) : 0.0;

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  out << "<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"UTF-8\">\n";
  out << "<title>HeatXTwin Simulation Report</title>\n";
  out << "<style>\n"
         "  body{font-family:'Segoe UI',Roboto,sans-serif;max-width:960px;margin:2em auto;color:#222;line-height:1.45;padding:0 1em;}\n"
         "  h1{color:#2c3e50;border-bottom:3px solid #3498db;padding-bottom:6px;}\n"
         "  h2{color:#2c3e50;margin-top:1.6em;border-left:4px solid #3498db;padding-left:10px;}\n"
         "  table{border-collapse:collapse;width:100%;margin:8px 0 18px 0;}\n"
         "  th,td{border:1px solid #d5dae0;padding:6px 10px;text-align:left;font-size:10.5pt;}\n"
         "  th{background:#ecf0f1;width:40%;font-weight:600;color:#34495e;}\n"
         "  td{background:#fdfdfe;}\n"
         "  .u{color:#7f8c8d;font-size:9.5pt;margin-left:4px;}\n"
         "  .kpi{display:inline-block;min-width:180px;padding:10px 14px;margin:4px 6px 4px 0;\n"
         "       background:#f4f8fb;border-left:4px solid #3498db;border-radius:2px;}\n"
         "  .kpi .lbl{font-size:9pt;color:#7f8c8d;text-transform:uppercase;letter-spacing:0.04em;}\n"
         "  .kpi .val{font-size:15pt;color:#2c3e50;font-weight:600;font-family:'Cambria Math',serif;}\n"
         "  .formula{font-family:'Cambria Math','Latin Modern Math',serif;font-style:italic;color:#2c3e50;\n"
         "           background:#fafbfc;padding:4px 8px;border-radius:3px;display:inline-block;}\n"
         "  footer{margin-top:2.4em;padding-top:10px;border-top:1px solid #d5dae0;color:#7f8c8d;font-size:9pt;text-align:center;}\n"
         "</style></head><body>\n";

  out << "<h1>HeatXTwin Digital Twin - Simulation Report</h1>\n";
  out << "<p>Generated <b>"
      << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
      << "</b> &middot; Mode <b>" << simulationModeLabel(simulationMode_)
      << "</b> &middot; Duration <b>" << fmt(t_final, 4) << " s</b> &middot; "
      << "<b>" << simulationData_.size() << "</b> samples</p>\n";

  out << "<h2>Key Performance Indicators (final state)</h2>\n";
  out << "<div>";
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">Heat duty Q</div><div class=\"val\">%1 W</div></div>").arg(fmt(s_final.Q, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">Overall U</div><div class=\"val\">%1 W/m&sup2;&middot;K</div></div>").arg(fmt(s_final.U, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">Effectiveness &epsilon;</div><div class=\"val\">%1</div></div>").arg(fmt(eps, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">NTU</div><div class=\"val\">%1</div></div>").arg(fmt(NTU, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">C<sub>r</sub> = C<sub>min</sub>/C<sub>max</sub></div><div class=\"val\">%1</div></div>").arg(fmt(Cr, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">Fouling penalty</div><div class=\"val\">%1 %</div></div>").arg(fmt(foulingPenalty, 3));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">&Delta;P tube</div><div class=\"val\">%1 Pa</div></div>").arg(fmt(s_final.dP_tube, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">&Delta;P shell</div><div class=\"val\">%1 Pa</div></div>").arg(fmt(s_final.dP_shell, 4));
  out << QStringLiteral("<div class=\"kpi\"><div class=\"lbl\">R<sub>f</sub></div><div class=\"val\">%1 m&sup2;&middot;K/W</div></div>").arg(fmt(s_final.Rf, 3));
  out << "</div>\n";
  out << "<p>&epsilon; computed from <span class=\"formula\">&epsilon; = Q / (C<sub>min</sub>&middot;&Delta;T<sub>in</sub>)</span>; "
         "NTU from <span class=\"formula\">NTU = UA / C<sub>min</sub></span>.</p>\n";

  out << "<h2>Flow arrangement &amp; fluid models</h2><table>\n";
  out << htmlRow("Flow arrangement",  arrangementLabel(simConfig_.arrangement));
  out << htmlRow("Hot-side model",    fluidPresetLabel(simConfig_.hotPreset));
  out << htmlRow("Cold-side model",   fluidPresetLabel(simConfig_.coldPreset));
  out << htmlRow("PID control loop",  simConfig_.pid.enabled ? QStringLiteral("ENABLED - regulating m&#775;<sub>cold</sub> to track T<sub>c,out</sub>")
                                                              : QStringLiteral("Disabled"));
  if (simConfig_.pid.enabled) {
    out << htmlRow("PID setpoint T<sub>c,out</sub>", fmt(simConfig_.pid.setpoint_Tc_out, 3), "&deg;C");
    out << htmlRow("PID gains (Kp, Ki, Kd)",
                    QStringLiteral("%1 / %2 / %3").arg(fmt(simConfig_.pid.kp,3), fmt(simConfig_.pid.ki,3), fmt(simConfig_.pid.kd,3)));
    out << htmlRow("Actuator limits",
                    QStringLiteral("[%1, %2]").arg(fmt(simConfig_.pid.u_min,3), fmt(simConfig_.pid.u_max,3)), "kg/s");
  }
  out << "</table>\n";

  out << "<h2>Operating point (initial)</h2><table>\n";
  out << htmlRow("m&#775;<sub>hot</sub>",   fmt(op_.m_dot_hot, 4),  "kg/s");
  out << htmlRow("m&#775;<sub>cold</sub>",  fmt(op_.m_dot_cold, 4), "kg/s");
  out << htmlRow("T<sub>in,hot</sub>",      fmt(op_.Tin_hot, 4),    "&deg;C");
  out << htmlRow("T<sub>in,cold</sub>",     fmt(op_.Tin_cold, 4),   "&deg;C");
  out << htmlRow("T<sub>out,hot</sub> (final)", fmt(s_final.Th_out, 4), "&deg;C");
  out << htmlRow("T<sub>out,cold</sub> (final)", fmt(s_final.Tc_out, 4), "&deg;C");
  out << htmlRow("Hot-side &Delta;T",       fmt(dT_hot, 4),         "K");
  out << htmlRow("Cold-side &Delta;T",      fmt(dT_cold, 4),        "K");
  out << htmlRow("C<sub>h</sub> = m&#775;<sub>h</sub>&middot;c<sub>p,h</sub>", fmt(Ch, 4), "W/K");
  out << htmlRow("C<sub>c</sub> = m&#775;<sub>c</sub>&middot;c<sub>p,c</sub>", fmt(Cc, 4), "W/K");
  out << "</table>\n";

  out << "<h2>Geometry</h2><table>\n";
  out << htmlRow("Number of tubes",   QString::number(geom_.nTubes));
  out << htmlRow("Tube ID / OD",      QStringLiteral("%1 / %2").arg(fmt(geom_.Di*1000,4), fmt(geom_.Do*1000,4)), "mm");
  out << htmlRow("Tube length",       fmt(geom_.L, 4),         "m");
  out << htmlRow("Pitch",             fmt(geom_.pitch*1000, 4),"mm");
  out << htmlRow("Shell ID",          fmt(geom_.shellID*1000,4),"mm");
  out << htmlRow("Baffle spacing",    fmt(geom_.baffleSpacing*1000,4), "mm");
  out << htmlRow("Baffle cut fraction", fmt(geom_.baffleCutFrac, 3));
  out << htmlRow("Number of baffles", QString::number(geom_.nBaffles));
  out << htmlRow("Wall k / thickness",
                  QStringLiteral("%1 W/m&middot;K / %2 mm").arg(fmt(geom_.wall_k,4), fmt(geom_.wall_thickness*1000,4)));
  out << htmlRow("Outer heat-transfer area A", fmt(geom_.areaOuter(), 4), "m&sup2;");
  out << "</table>\n";

  out << "<h2>Fluid properties (as supplied)</h2><table>\n";
  out << htmlRow("Hot &rho; / &mu; / c<sub>p</sub> / k",
                  QStringLiteral("%1 kg/m&sup3; / %2 Pa&middot;s / %3 J/kg&middot;K / %4 W/m&middot;K")
                    .arg(fmt(hot_.rho,4), fmt(hot_.mu,4), fmt(hot_.cp,4), fmt(hot_.k,4)));
  out << htmlRow("Cold &rho; / &mu; / c<sub>p</sub> / k",
                  QStringLiteral("%1 kg/m&sup3; / %2 Pa&middot;s / %3 J/kg&middot;K / %4 W/m&middot;K")
                    .arg(fmt(cold_.rho,4), fmt(cold_.mu,4), fmt(cold_.cp,4), fmt(cold_.k,4)));
  out << "</table>\n";
  if (simConfig_.hotPreset != hx::FluidPreset::Custom || simConfig_.coldPreset != hx::FluidPreset::Custom) {
    out << "<p><i>Values above are user inputs; temperature-dependent correlations were evaluated each step at the mean film temperature T = (T<sub>in</sub> + T<sub>out</sub>)/2.</i></p>\n";
  }

  out << "<h2>Fouling model</h2><table>\n";
  out << htmlRow("Model", foulParams_.model == hx::FoulingParams::Model::Asymptotic
                           ? QStringLiteral("Kern-Seaton asymptotic: R<sub>f</sub>(t) = R<sub>f,&infin;</sub>(1 - e<sup>-t/&tau;</sup>)")
                           : QStringLiteral("Linear: R<sub>f</sub>(t) = &alpha;&middot;t"));
  out << htmlRow("R<sub>f,&infin;</sub>", fmt(foulParams_.RfMax, 4), "m&sup2;&middot;K/W");
  out << htmlRow("&tau;",                 fmt(foulParams_.tau, 4),   "s");
  out << htmlRow("&alpha;",               fmt(foulParams_.alpha, 4), "m&sup2;&middot;K/(W&middot;s)");
  out << htmlRow("Deposit k",             fmt(foulParams_.k_deposit, 3), "W/m&middot;K");
  out << htmlRow("Shell/Tube split",
                  QStringLiteral("%1% shell / %2% tube")
                    .arg(fmt(100.0*foulParams_.split_ratio,3), fmt(100.0*(1.0-foulParams_.split_ratio),3)));
  out << "</table>\n";

  out << "<h2>Operating limits &amp; margins</h2><table>\n";
  const double mTube  = (simConfig_.limits.dP_tube_max  > 0.0) ? 100.0*(1.0 - s_final.dP_tube/simConfig_.limits.dP_tube_max)  : 0.0;
  const double mShell = (simConfig_.limits.dP_shell_max > 0.0) ? 100.0*(1.0 - s_final.dP_shell/simConfig_.limits.dP_shell_max) : 0.0;
  out << htmlRow("&Delta;P tube limit", fmt(simConfig_.limits.dP_tube_max, 4),  "Pa");
  out << htmlRow("&Delta;P shell limit",fmt(simConfig_.limits.dP_shell_max, 4), "Pa");
  out << htmlRow("Tube &Delta;P margin",  fmt(mTube, 3),  "% of limit remaining");
  out << htmlRow("Shell &Delta;P margin", fmt(mShell, 3), "% of limit remaining");
  out << "</table>\n";

  out << "<footer>Produced by HeatXTwin Pro 2.0 - educational digital twin of a shell-and-tube heat exchanger.<br>"
         "Formulas follow Incropera/DeWitt, Kern, and Bowman (1936); see source for references.</footer>\n";
  out << "</body></html>\n";
  file.close();

  statusBar()->showMessage(QString("Report written to %1").arg(filename), 5000);
  const auto reply = QMessageBox::information(this, "Report Generated",
      QStringLiteral("HTML report saved to:\n%1\n\nOpen it now in the default browser?").arg(filename),
      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
  }
}

QString MainWindow::simulationModeLabel(SimulationMode mode) {
  switch (mode) {
    case SimulationMode::SteadyClean:
      return QStringLiteral("Steady Clean (no fouling)");
    case SimulationMode::SteadyFouling:
      return QStringLiteral("Steady with Fouling");
    case SimulationMode::DynamicClean:
      return QStringLiteral("Dynamic Clean (disturbances)");
    case SimulationMode::DynamicFouling:
    default:
      return QStringLiteral("Dynamic with Fouling");
  }
}

void MainWindow::applySimulationModeToUi() {
  if (cmbSimulationMode_) {
    QSignalBlocker blocker(cmbSimulationMode_);
    cmbSimulationMode_->setCurrentIndex(static_cast<int>(simulationMode_));
  }

  const ModeConfig cfg = modeConfig(simulationMode_);

  if (grpFouling_) {
    const QString title = cfg.fouling
                             ? QStringLiteral("Fouling Model")
                             : QStringLiteral("Fouling Model (disabled)");
    grpFouling_->setTitle(title);
    grpFouling_->setEnabled(cfg.fouling);
    grpFouling_->setToolTip(cfg.fouling ? QString() : QStringLiteral("Fouling is disabled in this preset."));
  }
}

void MainWindow::onSimulationModeChanged(int index) {
  const int clamped = std::clamp(index, 0, 3);
  simulationMode_ = static_cast<SimulationMode>(clamped);
  applySimulationModeToUi();

  const QString modeName = simulationModeLabel(simulationMode_);
  if (isRunning_) {
    statusBar()->showMessage(QStringLiteral("Mode will apply on next start: %1").arg(modeName), 5000);
  } else {
    statusBar()->showMessage(QStringLiteral("Simulation mode set to %1").arg(modeName), 4000);
  }
}

void MainWindow::onGeometryDebounceTimeout() {
  validateAndUpdateGeometry();
}

void MainWindow::validateAndUpdateGeometry() {
  // Validate geometry parameters before updating
  QString errorMsg;
  if (!validateGeometryParameters(geom_, errorMsg)) {
    QMessageBox::warning(this, "Invalid Geometry",
                        QString("Geometry validation failed:\n\n%1").arg(errorMsg));
    statusBar()->showMessage("Invalid geometry parameters - please correct them", 5000);
    return;
  }
  
  // All validations passed - safe to update
  updateSimulationCore();
  statusBar()->showMessage("Geometry updated successfully", 3000);
}

bool MainWindow::validateGeometryParameters(const hx::Geometry &geom, QString &errorMsg) {
  // Validate tube parameters
  if (geom.nTubes < 1) {
    errorMsg = "Number of tubes must be at least 1";
    return false;
  }
  
  if (geom.nTubes > 5000) {
    errorMsg = "Number of tubes exceeds maximum (5000)";
    return false;
  }
  
  if (geom.Di <= 0.0 || geom.Do <= 0.0) {
    errorMsg = "Tube diameters must be positive";
    return false;
  }
  
  if (geom.Do <= geom.Di) {
    errorMsg = "Outer diameter must be greater than inner diameter";
    return false;
  }
  
  if (geom.L <= 0.0) {
    errorMsg = "Tube length must be positive";
    return false;
  }
  
  // Validate pitch vs tube diameter
  if (geom.pitch < geom.Do * 1.1) {
    errorMsg = QString("Pitch (%.1f mm) too small for tube diameter (%.1f mm) - minimum is %.1f mm")
      .arg(geom.pitch * 1000, 0, 'f', 1)
      .arg(geom.Do * 1000, 0, 'f', 1)
      .arg(geom.Do * 1100, 0, 'f', 1);
    return false;
  }
  
  // Validate shell parameters
  if (geom.shellID <= 0.0) {
    errorMsg = "Shell inner diameter must be positive";
    return false;
  }
  
  if (geom.shellID <= geom.Do) {
    errorMsg = "Shell diameter must be larger than tube outer diameter";
    return false;
  }
  
  // Validate baffle parameters
  if (geom.nBaffles < 0) {
    errorMsg = "Number of baffles cannot be negative";
    return false;
  }
  
  if (geom.baffleSpacing <= 0.0) {
    errorMsg = "Baffle spacing must be positive";
    return false;
  }
  
  if (geom.baffleCutFrac < 0.15 || geom.baffleCutFrac > 0.45) {
    errorMsg = "Baffle cut fraction must be between 0.15 and 0.45";
    return false;
  }
  
  // Validate wall parameters
  if (geom.wall_thickness <= 0.0) {
    errorMsg = "Wall thickness must be positive";
    return false;
  }
  
  if (geom.wall_k <= 0.0) {
    errorMsg = "Wall thermal conductivity must be positive";
    return false;
  }
  
  // All checks passed
  return true;
}

