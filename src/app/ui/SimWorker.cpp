#include "SimWorker.hpp"
#include <QThread>
#include <algorithm>
#include <cmath>

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

  // Real-time pacing: each simulated dt should take dt·1000/speed ms in wall
  // time.  At high speed × small dt the ideal delay collapses to <5 ms,
  // which would push 200+ Hz sampleReady signals at the UI thread and make
  // the charts / KPIs / exchanger widget unresponsive (and eventually blow
  // out the event queue).
  const double idealDelayMs = (dt * 1000.0) / std::max(1, speedMultiplier_);

  // UI-emit cap: never fire sampleReady more than ~50 Hz (20 ms gap).  When
  // the ideal delay is below that we batch multiple simulation steps into a
  // single sleep+emit, so wall-clock pacing is preserved while the UI gets
  // a sane update rate.
  constexpr int  kMinEmitMs      = 20;
  const int      stepsPerEmit    = (idealDelayMs < kMinEmitMs)
                                     ? std::max(1, static_cast<int>(std::ceil(kMinEmitMs / std::max(idealDelayMs, 0.1))))
                                     : 1;
  const int      delayMs         = std::max(1,
                                     static_cast<int>(idealDelayMs * stepsPerEmit));

  int stepCounter = 0;
  hx::State lastState{};
  while (t < tEnd && !stopRequested_) {
    lastState = simulator_->step(t);
    t += dt;
    ++stepCounter;

    // Batch emit: only notify the UI every `stepsPerEmit` simulator steps.
    if (stepCounter >= stepsPerEmit) {
      emit sampleReady(t - dt, lastState);
      stepCounter = 0;

      int percent = static_cast<int>((t / tEnd) * 100.0);
      if (percent != lastPercent) {
        emit progress(percent);
        lastPercent = percent;
      }

      QThread::msleep(static_cast<unsigned long>(delayMs));
    }
  }

  // Final sample if the last batch didn't align exactly.
  if (stepCounter > 0) emit sampleReady(t - dt, lastState);

  emit finished();
}

void SimWorker::stop() {
  stopRequested_ = true;
}
