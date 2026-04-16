#include "Scenario.hpp"

namespace hx {

namespace {
// Helper builders so the factory functions below read like a timeline.
ScenarioEvent mark(double t, const char *note) {
  ScenarioEvent e;
  e.t = t;
  e.action = ScenarioEvent::Action::Mark;
  e.note = note;
  return e;
}
ScenarioEvent setHotInlet(double t, double C, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetHotInletT;
  e.value = C; e.note = note; return e;
}
ScenarioEvent setColdInlet(double t, double C, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetColdInletT;
  e.value = C; e.note = note; return e;
}
ScenarioEvent setHotFlow(double t, double kgs, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetHotFlow;
  e.value = kgs; e.note = note; return e;
}
ScenarioEvent setColdFlow(double t, double kgs, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetColdFlow;
  e.value = kgs; e.note = note; return e;
}
ScenarioEvent setSetpoint(double t, double C, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetPidSetpoint;
  e.value = C; e.note = note; return e;
}
ScenarioEvent setPid(double t, bool on, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::SetPidEnabled;
  e.boolValue = on; e.note = note; return e;
}
ScenarioEvent foulingJump(double t, double dRf, const char *note = "") {
  ScenarioEvent e; e.t = t; e.action = ScenarioEvent::Action::FoulingJump;
  e.value = dRf; e.note = note; return e;
}
} // namespace

Scenario scenarioStartupRamp() {
  // Cold start: hot feed warms from 40 °C to 90 °C over a few minutes,
  // cold side is brought online after the hot loop has stabilised, and
  // the PID is handed control at the end.
  return {
    mark(0.0,    "Startup — hot loop slowly heating"),
    setHotInlet (  0.0, 40.0),
    setHotFlow  (  0.0, 0.8),
    setColdFlow (  0.0, 0.2),
    setPid      (  0.0, false, "Manual mode during warm-up"),
    setHotInlet ( 60.0, 55.0, "Hot inlet +15 °C"),
    setHotInlet (180.0, 75.0, "Hot inlet +20 °C"),
    setColdFlow (240.0, 1.0,  "Cold loop brought online"),
    setHotInlet (300.0, 90.0, "Hot inlet at nominal"),
    setSetpoint (360.0, 50.0, "Handover setpoint"),
    setPid      (360.0, true, "Controller in automatic"),
    mark       (420.0,        "Startup ramp complete"),
  };
}

Scenario scenarioFeedUpset() {
  // Plant in normal operation suffers a feed disturbance around t=180 s:
  // hot feed drops by 20 % and simultaneously heats by +8 °C for 60 s.
  // Recovery event restores nominals at t=420 s.
  return {
    mark(0.0,    "Nominal operation — wait for disturbance"),
    setHotFlow  (  0.0, 1.0),
    setHotInlet (  0.0, 80.0),
    setColdFlow (  0.0, 1.0),
    setPid      (  0.0, true),
    setSetpoint (  0.0, 45.0),
    setHotFlow  (180.0, 0.8,  "FEED UPSET: hot flow -20 %"),
    setHotInlet (180.0, 88.0, "FEED UPSET: hot inlet +8 °C"),
    setHotFlow  (420.0, 1.0,  "Recovery: hot flow restored"),
    setHotInlet (420.0, 80.0, "Recovery: hot inlet restored"),
    mark       (540.0,        "Upset/recovery sequence done"),
  };
}

Scenario scenarioFoulingBurst() {
  // Sudden fouling event on top of the slow background growth.  Useful
  // for showing the KPI "fouling penalty" swinging negative and the
  // controller compensating by opening the cold valve.
  return {
    mark(0.0,    "Clean plant — fouling event in 2 minutes"),
    setPid      (  0.0, true),
    setSetpoint (  0.0, 45.0),
    setHotFlow  (  0.0, 1.0),
    setColdFlow (  0.0, 1.0),
    setHotInlet (  0.0, 80.0),
    foulingJump (120.0, 1.5e-4, "FOULING BURST #1 (+1.5e-4)"),
    foulingJump (240.0, 1.5e-4, "FOULING BURST #2 (+1.5e-4)"),
    foulingJump (360.0, 1.5e-4, "FOULING BURST #3 (+1.5e-4)"),
    mark       (480.0,          "Three bursts accumulated"),
  };
}

Scenario scenarioPidChallenge() {
  // Classic controller test: large setpoint step, then a disturbance
  // while at the new setpoint, then a step back.  Good demo for judging
  // tracking vs regulation performance of whatever gains are active.
  return {
    mark(0.0,    "PID challenge — step-then-disturb"),
    setPid      (  0.0, true),
    setSetpoint (  0.0, 40.0),
    setHotFlow  (  0.0, 1.0),
    setColdFlow (  0.0, 1.0),
    setHotInlet (  0.0, 80.0),
    setSetpoint (120.0, 55.0, "Setpoint step +15 °C"),
    setHotInlet (360.0, 88.0, "Disturbance: Tin,hot +8 °C"),
    setHotInlet (540.0, 80.0, "Disturbance removed"),
    setSetpoint (660.0, 45.0, "Setpoint step -10 °C"),
    mark       (780.0,        "Challenge complete"),
  };
}

Scenario scenarioCleaningCycle() {
  // Emulates an operator scheduling a cleaning:
  // - build fouling up
  // - trip the unit (flows to min)
  // - "clean" by zeroing fouling (large negative jump)
  // - bring it back online.
  return {
    mark(0.0,    "Production → cleaning → restart"),
    setPid      (  0.0, true),
    setSetpoint (  0.0, 45.0),
    setHotFlow  (  0.0, 1.0),
    setColdFlow (  0.0, 1.0),
    setHotInlet (  0.0, 80.0),
    foulingJump (120.0, 2.0e-4, "Fouling ramp-up"),
    foulingJump (240.0, 2.0e-4, "Fouling ramp-up"),
    setPid      (360.0, false,  "Unit TRIP — manual mode"),
    setHotFlow  (360.0, 0.1,    "Hot flow reduced to minimum"),
    setColdFlow (360.0, 0.1,    "Cold flow reduced to minimum"),
    mark       (420.0,          "Cleaning in progress…"),
    foulingJump (480.0, -1.0,   "Cleaning complete — Rf reset"),
    setHotFlow  (540.0, 1.0,    "Bringing unit back online"),
    setColdFlow (540.0, 1.0),
    setPid      (600.0, true,   "Controller back to automatic"),
    mark       (720.0,          "Cleaning cycle finished"),
  };
}

const char *scenarioName(int idx) {
  switch (idx) {
    case 0: return "None";
    case 1: return "Startup Ramp";
    case 2: return "Feed Upset";
    case 3: return "Fouling Burst";
    case 4: return "PID Challenge (step + disturb)";
    case 5: return "Cleaning Cycle";
    default: return "?";
  }
}

Scenario scenarioByIndex(int idx) {
  switch (idx) {
    case 1: return scenarioStartupRamp();
    case 2: return scenarioFeedUpset();
    case 3: return scenarioFoulingBurst();
    case 4: return scenarioPidChallenge();
    case 5: return scenarioCleaningCycle();
    case 0:
    default: return {};
  }
}

} // namespace hx
