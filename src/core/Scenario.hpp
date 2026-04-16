#pragma once

#include <string>
#include <vector>

namespace hx {

/**
 * \brief A single action taken at a specified simulated time.
 *
 * Events live inside SimConfig::scenario.  At the top of each Simulator::step
 * call any un-fired events whose trigger time has passed are applied, in the
 * order they were declared, and marked as fired so they don't re-trigger.
 *
 * The runtime impact of each Action is deliberately small — an event is just
 * a deferred mutation of the existing OperatingPoint / PidConfig / State
 * fields.  This keeps the scenario engine trivial to reason about while
 * letting us script rich demo stories ("startup → setpoint nudge → feed
 * upset → fouling burst → cleaning") without any new plant code.
 */
struct ScenarioEvent {
  double t = 0.0;                 // [s] simulated time at which to fire

  enum class Action {
    SetHotFlow,        // value = new m_dot_hot [kg/s]
    SetColdFlow,       // value = new m_dot_cold [kg/s]
    SetHotInletT,      // value = new Tin_hot [C]
    SetColdInletT,     // value = new Tin_cold [C]
    SetPidSetpoint,    // value = new Tc_out setpoint [C]
    SetPidEnabled,     // boolValue = true (enable) / false (disable)
    FoulingJump,       // value = additive bump to state.Rf [m²·K/W]
    Mark               // note appears in timeline log; plant unchanged
  };
  Action action = Action::Mark;

  double value     = 0.0;
  bool   boolValue = false;
  std::string note;            // human-readable description, shown in UI log
  mutable bool fired = false;  // runtime: set once the event has applied
};

using Scenario = std::vector<ScenarioEvent>;

/** Built-in demo scenarios.  Each assumes a reasonable starting operating
 *  point; the first event is always `Mark` so the UI can name the timeline. */
Scenario scenarioStartupRamp();
Scenario scenarioFeedUpset();
Scenario scenarioFoulingBurst();
Scenario scenarioPidChallenge();
Scenario scenarioCleaningCycle();

/** Human-readable name for the combo-box — keep in sync with the list. */
const char *scenarioName(int presetIndex);

/** Convenience: build preset by index 0..N-1 (0 = "None"). */
Scenario scenarioByIndex(int presetIndex);

constexpr int kNumScenarioPresets = 6;   // None + the five factories above

} // namespace hx
