#include "SimWorker.hpp"
#include <QThread>

SimWorker::SimWorker(QObject *parent) : QObject(parent) {}

void SimWorker::setSimulator(std::unique_ptr<hx::Simulator> sim, const hx::SimConfig& config) {
  simulator_ = std::move(sim);
  config_ = config;
}

void SimWorker::run() {
  if (!simulator_) {
    emit finished();
    return;
  }

  stopRequested_ = false;
  const double dt = config_.dt;
  const double tEnd = config_.tEnd;
  
  double t = 0.0;
  int lastPercent = 0;
  
  // Calculate delay to match real-time, adjusted by speed multiplier
  // dt is in seconds, so we sleep for (dt * 1000 / speedMultiplier) milliseconds
  // Speed 1x: dt=1.0s → sleep 1000ms (real-time)
  // Speed 10x: dt=1.0s → sleep 100ms (10x faster)
  // Speed 100x: dt=1.0s → sleep 10ms (100x faster)
  int delayMs = static_cast<int>((dt * 1000.0) / std::max(1, speedMultiplier_));
  
  // Ensure minimum delay for UI responsiveness (at least 1ms)
  delayMs = std::max(1, delayMs);
  
  while (t < tEnd && !stopRequested_) {
    // Step simulation
    const hx::State& state = simulator_->step(t);
    
    // Emit sample
    emit sampleReady(t, state);
    
    // Update progress
    int percent = static_cast<int>((t / tEnd) * 100.0);
    if (percent != lastPercent) {
      emit progress(percent);
      lastPercent = percent;
    }

    t += dt;
    
    // Delay based on speed multiplier
    QThread::msleep(delayMs);
  }

  emit finished();
}

void SimWorker::stop() {
  stopRequested_ = true;
}
