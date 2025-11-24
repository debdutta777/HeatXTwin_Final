# Build Fixes Summary

## 1. Compilation Errors Resolved
- **ChartWidget.cpp**: Fixed `undeclared identifier 'newMax'` error. The variable was being used inside an `if` block before its declaration. Reordered the logic to declare `newMax` before using it for fouling range adjustments.
- **Model.cpp**: Updated function calls to match new signatures in `Thermo` and `Hydraulics`.
  - Passed `k_deposit` to `thermo_.steady`.
  - Passed `k_deposit`, `K_minor_tube`, and `K_turns_shell` to `hydro_.dP_tube` and `hydro_.dP_shell`.
  - **Correction**: Fixed fluid assignment in `Model::evaluate`. Tube side now correctly uses `m_dot_hot` and Shell side uses `m_dot_cold`.
  - Implemented `split_ratio` logic for fouling distribution.
- **MainWindow.cpp**: Fixed typo `grpFouling` -> `grpFouling_` (missing underscore for member variable).

## 2. Build System Restoration
- **CMakeLists.txt**: Restored the missing CMake configuration file, defining the project structure and dependencies (Qt6, Eigen3, fmt, spdlog, tomlplusplus).
- **IntelliSense**: Updated `.vscode/c_cpp_properties.json` to use the MSVC compiler found on the system (`C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/...`) and switched to `windows-msvc-x64` mode. This resolved "cannot open source file" errors for standard headers.

## 3. Verification
- Successfully configured with `cmake -B build`.
- Successfully built with `cmake --build build --config Release`.
- Application launches successfully via `run_app.bat`.
