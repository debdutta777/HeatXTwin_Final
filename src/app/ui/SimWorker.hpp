#pragma once

#include <QObject>
#include <memory>
#include "core/Simulator.hpp"
#include "core/Types.hpp"

/**
 * @brief Background worker for running simulation
 * Runs in separate thread to keep UI responsive
 */
class SimWorker : public QObject {
  Q_OBJECT

public:
  explicit SimWorker(QObject *parent = nullptr);
  
  void setSimulator(std::unique_ptr<hx::Simulator> sim, const hx::SimConfig& config);
  void setSpeedMultiplier(int speed) { speedMultiplier_ = speed; }

public slots:
  void run();
  void stop();

signals:
  void sampleReady(double t, hx::State state);
  void finished();
  void progress(int percent);

private:
  std::unique_ptr<hx::Simulator> simulator_;
  hx::SimConfig config_{};
  bool stopRequested_{false};
  int speedMultiplier_{1};  // 1x = real-time, 100x = 100x faster
};
