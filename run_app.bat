@echo off
REM HeatXTwin Pro - Local Launch Script
echo ========================================
echo Starting HeatXTwin Pro...
echo ========================================
echo.

REM Check if build exists
if exist "build\Release\HeatXTwin_Pro.exe" (
    set "APP_DIR=build\Release"
) else if exist "build\bin\Release\HeatXTwin_Pro.exe" (
    set "APP_DIR=build\bin\Release"
) else (
    echo ERROR: Application not built yet!
    echo.
    echo Please build the application first:
    echo 1. Open CMake GUI or run: cmake -B build
    echo 2. Build: cmake --build build --config Release
    echo.
    pause
    exit /b 1
)

REM Navigate to the executable directory
cd %APP_DIR%

REM Launch the application
echo Starting HeatXTwin_Pro.exe...
echo.
start HeatXTwin_Pro.exe

echo Application launched!
echo You can close this window.
echo.
timeout /t 2
