# Deployment Fixes

## Issue
The application failed to start with the error: "This application failed to start because no Qt platform plugin could be initialized."

## Cause
The Qt runtime dependencies (DLLs and plugins) were not present in the executable directory. This is a common requirement for Qt applications on Windows.

## Fix
1.  **Updated `CMakeLists.txt`**:
    *   Added a `POST_BUILD` custom command to run `windeployqt.exe`.
    *   This tool automatically scans the executable and copies the necessary Qt DLLs (Core, Gui, Widgets, Charts) and plugins (platforms/qwindows.dll, styles, etc.) to the build directory.
    *   Configured it to look for `windeployqt` in the vcpkg installation directory.

2.  **Updated `run_app.bat`**:
    *   Modified the script to check both `build\Release` and `build\bin\Release` for the executable, making it more robust to different CMake generator defaults.

## Verification
*   Ran `cmake --build` which triggered the deployment step.
*   Verified `platforms/qwindows.dll` exists in the output directory.
*   Launched the application successfully.
