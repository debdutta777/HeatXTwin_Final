#include "MainWindow.hpp"
#include "ChartWidget.hpp"
#include "SimWorker.hpp"
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
  setWindowTitle("HeatXTwin Pro - Modern Heat Exchanger Digital Twin");
  resize(1400, 900);
  
  resetToDefaults();  // Initialize with default values
  setupUi();
  applySimulationModeToUi();
  applyModernStyle();
  updateSimulationCore();
  
  // Remove margins from central widget for more space
  centralWidget()->setContentsMargins(0, 0, 0, 0);
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

  // Fluid properties (water)
  hot_.rho = 997.0;
  hot_.mu = 0.001;
  hot_.cp = 4180.0;
  hot_.k = 0.6;
  
  cold_ = hot_;  // Same fluid properties

  // Fouling parameters
  foulParams_.Rf0 = 0.0;
  foulParams_.RfMax = 0.0005;
  foulParams_.alpha = 1e-7;
  foulParams_.tau = 1800.0;
  foulParams_.model = hx::FoulingParams::Model::Asymptotic;

  // Simulation config
  simConfig_.dt = 2.0;
  simConfig_.tEnd = 1800.0;
  
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
  auto *grpOperating = new QGroupBox("🌡️ Operating Conditions", this);
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
  spnHotInletTemp_->setSuffix(" °C");
  connect(spnHotInletTemp_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Hot Inlet Temp:", spnHotInletTemp_);

  spnColdInletTemp_ = new QDoubleSpinBox(this);
  spnColdInletTemp_->setRange(5, 50);
  spnColdInletTemp_->setValue(op_.Tin_cold);
  spnColdInletTemp_->setSuffix(" °C");
  connect(spnColdInletTemp_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formOp->addRow("Cold Inlet Temp:", spnColdInletTemp_);

  layout->addWidget(grpOperating);

  // === HOT FLUID PROPERTIES ===
  auto *grpHotFluid = new QGroupBox("🔥 Hot Fluid Properties", this);
  auto *formHot = new QFormLayout(grpHotFluid);
  formHot->setSpacing(5);
  formHot->setContentsMargins(8, 8, 8, 8);
  formHot->setHorizontalSpacing(10);
  formHot->setVerticalSpacing(5);

  spnHotDensity_ = new QDoubleSpinBox(this);
  spnHotDensity_->setRange(500, 2000);
  spnHotDensity_->setValue(hot_.rho);
  spnHotDensity_->setSuffix(" kg/m³");
  connect(spnHotDensity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Density (ρ):", spnHotDensity_);

  spnHotViscosity_ = new QDoubleSpinBox(this);
  spnHotViscosity_->setRange(0.0001, 0.01);
  spnHotViscosity_->setDecimals(5);
  spnHotViscosity_->setSingleStep(0.00001);
  spnHotViscosity_->setValue(hot_.mu);
  spnHotViscosity_->setSuffix(" Pa·s");
  connect(spnHotViscosity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Viscosity (μ):", spnHotViscosity_);

  spnHotSpecificHeat_ = new QDoubleSpinBox(this);
  spnHotSpecificHeat_->setRange(1000, 10000);
  spnHotSpecificHeat_->setValue(hot_.cp);
  spnHotSpecificHeat_->setSuffix(" J/kg·K");
  connect(spnHotSpecificHeat_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Specific Heat (cp):", spnHotSpecificHeat_);

  spnHotConductivity_ = new QDoubleSpinBox(this);
  spnHotConductivity_->setRange(0.01, 5.0);
  spnHotConductivity_->setDecimals(3);
  spnHotConductivity_->setValue(hot_.k);
  spnHotConductivity_->setSuffix(" W/m·K");
  connect(spnHotConductivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formHot->addRow("Conductivity (k):", spnHotConductivity_);

  layout->addWidget(grpHotFluid);

  // === COLD FLUID PROPERTIES ===
  auto *grpColdFluid = new QGroupBox("❄️ Cold Fluid Properties", this);
  auto *formCold = new QFormLayout(grpColdFluid);
  formCold->setSpacing(5);
  formCold->setContentsMargins(8, 8, 8, 8);
  formCold->setHorizontalSpacing(10);
  formCold->setVerticalSpacing(5);

  spnColdDensity_ = new QDoubleSpinBox(this);
  spnColdDensity_->setRange(500, 2000);
  spnColdDensity_->setValue(cold_.rho);
  spnColdDensity_->setSuffix(" kg/m³");
  connect(spnColdDensity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Density (ρ):", spnColdDensity_);

  spnColdViscosity_ = new QDoubleSpinBox(this);
  spnColdViscosity_->setRange(0.0001, 0.01);
  spnColdViscosity_->setDecimals(5);
  spnColdViscosity_->setSingleStep(0.00001);
  spnColdViscosity_->setValue(cold_.mu);
  spnColdViscosity_->setSuffix(" Pa·s");
  connect(spnColdViscosity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Viscosity (μ):", spnColdViscosity_);

  spnColdSpecificHeat_ = new QDoubleSpinBox(this);
  spnColdSpecificHeat_->setRange(1000, 10000);
  spnColdSpecificHeat_->setValue(cold_.cp);
  spnColdSpecificHeat_->setSuffix(" J/kg·K");
  connect(spnColdSpecificHeat_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Specific Heat (cp):", spnColdSpecificHeat_);

  spnColdConductivity_ = new QDoubleSpinBox(this);
  spnColdConductivity_->setRange(0.01, 5.0);
  spnColdConductivity_->setDecimals(3);
  spnColdConductivity_->setValue(cold_.k);
  spnColdConductivity_->setSuffix(" W/m·K");
  connect(spnColdConductivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formCold->addRow("Conductivity (k):", spnColdConductivity_);

  layout->addWidget(grpColdFluid);

  // === GEOMETRY ===
  auto *grpGeometry = new QGroupBox("📐 Geometry", this);
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
  spnWallConductivity_->setSuffix(" W/m·K");
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
  spnRf0_->setSuffix(" m²K/W");
  connect(spnRf0_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onParameterChanged);
  formFoul->addRow("Initial Rf (Rf0):", spnRf0_);

  spnRfMax_ = new QDoubleSpinBox(this);
  spnRfMax_->setRange(0.0, 0.01);
  spnRfMax_->setDecimals(5);
  spnRfMax_->setSingleStep(0.0001);
  spnRfMax_->setValue(foulParams_.RfMax);
  spnRfMax_->setSuffix(" m²K/W");
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
  spnAlpha_->setSuffix(" m²K/(W·s)");
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

  // === LIMITS ===
  auto *grpLimits = new QGroupBox("⚠️ Operating Limits", this);
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

  // Add to tabs
  chartTabs_->addTab(chartTemp_, "🌡️ Temperatures");
  chartTabs_->addTab(chartHeat_, "🔥 Heat Duty & U");
  chartTabs_->addTab(chartPressure_, "💧 Pressure Drops");
  chartTabs_->addTab(chartFouling_, "⚙️ Fouling");
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

  // Recreate simulation objects with updated parameters
  thermo_ = std::make_unique<hx::Thermo>(geom_, hot_, cold_);
  hydro_ = std::make_unique<hx::Hydraulics>(geom_, hot_, cold_);
  fouling_ = std::make_unique<hx::Fouling>(foulParams_);
}

void MainWindow::onParameterChanged() {
  if (!isRunning_) {
    updateSimulationCore();
  }
}

void MainWindow::onStart() {
  updateSimulationCore();

  // Clear all charts and data
  chartTemp_->clear();
  chartHeat_->clear();
  chartPressure_->clear();
  chartFouling_->clear();
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
  isPaused_ = false;
  lblStatus_->setText("Running");
  lblStatus_->setStyleSheet("color: #e67e22; font-weight: bold; font-size: 10pt;");
  isRunning_ = true;

  // Start
  simThread_->start();
  statusBar()->showMessage("Simulation running...");
}

void MainWindow::onStop() {
  if (simWorker_) {
    simWorker_->stop();
  }
  
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
}

void MainWindow::onSimulationFinished() {
  btnStart_->setEnabled(true);
  btnPause_->setEnabled(false);
  btnStop_->setEnabled(false);
  btnExport_->setEnabled(true);
  lblStatus_->setText("● Complete");
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
  }
}

void MainWindow::onZoomOut() {
  int currentTab = chartTabs_->currentIndex();
  switch (currentTab) {
    case 0: chartTemp_->zoomOut(); break;
    case 1: chartHeat_->zoomOut(); break;
    case 2: chartPressure_->zoomOut(); break;
    case 3: chartFouling_->zoomOut(); break;
  }
}

void MainWindow::onZoomReset() {
  int currentTab = chartTabs_->currentIndex();
  switch (currentTab) {
    case 0: chartTemp_->resetZoom(); break;
    case 1: chartHeat_->resetZoom(); break;
    case 2: chartPressure_->resetZoom(); break;
    case 3: chartFouling_->resetZoom(); break;
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


