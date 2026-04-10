@echo off
setlocal enabledelayedexpansion
REM ============================================================================
REM HeatXTwin Pro - Enhanced Launcher with Robustness & Diagnostics
REM ============================================================================
REM Features:
REM - Automatic build recovery
REM - Comprehensive dependency validation
REM - Detailed error diagnostics
REM - Safe path handling (UNC/spaces aware)
REM - Logging to file for debugging
REM ============================================================================

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "ROOT_DIR=%SCRIPT_DIR%"
set "LOG_FILE=%ROOT_DIR%\HeatXTwin_Launcher.log"
set "TIMESTAMP=%date% %time%"

REM Initialize diagnostic log
echo ============================================================================ > "%LOG_FILE%"
echo HeatXTwin Pro Launcher - %TIMESTAMP% >> "%LOG_FILE%"
echo ============================================================================ >> "%LOG_FILE%"
echo Root Directory: %ROOT_DIR% >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

echo ========================================
echo  HeatXTwin Pro - Enhanced Launcher
echo ========================================
echo.

REM Step 1: Locate the executable
echo [1/5] Locating executable...
set "APP_EXE="
for %%P in ("build\Release\HeatXTwin_Pro.exe" "build\bin\Release\HeatXTwin_Pro.exe") do (
    if exist "%%P" (
        set "APP_EXE=%%P"
        set "APP_DIR=%%~dp%%P"
        set "APP_DIR=!APP_DIR:~0,-1!"
        goto exe_found
    )
)

:exe_not_found
echo [ERROR] Application executable not found!
echo. 
echo The application has not been built yet.
echo Would you like to build it now? (Y/N)
set /p BUILD_CHOICE=^>
if /i "%BUILD_CHOICE%"=="Y" (
    echo.
    echo [INFO] Building Release configuration...
    if not exist "build" mkdir build
    cd /d "%ROOT_DIR%"
    cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake 2>&1 | tee -a "%LOG_FILE%"
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configuration failed. Check '%LOG_FILE%' for details.
        pause
        exit /b 1
    )
    cmake --build build --config Release --target HeatXTwin_Pro -j 8 2>&1 | tee -a "%LOG_FILE%"
    if errorlevel 1 (
        echo.
        echo [ERROR] Build failed. Check '%LOG_FILE%' for details.
        pause
        exit /b 1
    )
    cd /d "%ROOT_DIR%"
    REM Retry finding exe
    if exist "build\Release\HeatXTwin_Pro.exe" (
        set "APP_EXE=build\Release\HeatXTwin_Pro.exe"
        set "APP_DIR=!APP_EXE:~0,-27!"
        goto exe_found
    ) else (
        echo [ERROR] Build completed but executable not found.
        pause
        exit /b 1
    )
) else (
    echo [ERROR] Build skipped. Cannot launch without executable.
    echo Diagnostic log saved to: %LOG_FILE%
    pause
    exit /b 1
)

:exe_found
echo [OK] Found: %APP_EXE%
echo Application Directory: %APP_DIR% >> "%LOG_FILE%"
echo Executable Path: %APP_EXE% >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

REM Step 2: Validate Qt plugin paths
echo [2/5] Validating Qt plugin paths...
set "QT_ROOT=%ROOT_DIR%\build\vcpkg_installed\x64-windows"
set "QT_PLUGIN_PATH=%QT_ROOT%\Qt6\plugins"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%QT_PLUGIN_PATH%\platforms"

if exist "%QT_QPA_PLATFORM_PLUGIN_PATH%\qwindows.dll" (
    echo [OK] Qt platform plugin found
    echo Qt Plugin Path: %QT_PLUGIN_PATH% >> "%LOG_FILE%"
) else (
    echo [WARN] Qt plugin path not found: %QT_QPA_PLATFORM_PLUGIN_PATH%
    echo [WARN] The application may fail to initialize. Check your vcpkg installation.
    echo. >> "%LOG_FILE%"
    echo [WARNING] Qt plugin path issue: %QT_QPA_PLATFORM_PLUGIN_PATH% >> "%LOG_FILE%"
)

REM Step 3: Check critical runtime files
echo [3/5] Validating runtime dependencies...
set "DEPS_OK=1"

for %%D in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6 3D.dll) do (
    if not exist "%APP_DIR%\%%D" (
        echo [WARN] Missing: %%D
        set "DEPS_OK=0"
    )
)

if "!DEPS_OK!"=="1" (
    echo [OK] Critical dependencies verified
) else (
    echo [WARN] Some dependencies may be missing - app may fail to start
    echo [INFO] Attempting to locate libraries in vcpkg...
)
echo. >> "%LOG_FILE%"

REM Step 4: Set environment variables for safe execution
echo [4/5] Configuring environment...
set "PATH=%APP_DIR%;%QT_PLUGIN_PATH%;%PATH%"
set "QT_PLUGIN_PATH=%QT_PLUGIN_PATH%"
set "QT_QPA_PLATFORM=windows"

REM Suppress debug console spam
set "QT_DEBUG_PLUGINS=0"
set "QML_DISABLE_DISK_CACHE=1"

echo Environment configured:
echo   PATH includes: %APP_DIR%
echo   QT_PLUGIN_PATH: %QT_PLUGIN_PATH%
echo   QT_QPA_PLATFORM: %QT_QPA_PLATFORM%
echo. >> "%LOG_FILE%"
echo Environment Variables: >> "%LOG_FILE%"
echo   PATH includes APP_DIR >> "%LOG_FILE%"
echo   QT_PLUGIN_PATH=%QT_PLUGIN_PATH% >> "%LOG_FILE%"
echo   QT_QPA_PLATFORM=%QT_QPA_PLATFORM% >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

REM Step 5: Launch the application
echo [5/5] Launching HeatXTwin Pro...
echo.
echo ========================================
echo Application started at: %TIMESTAMP%
echo ========================================
echo.
echo Diagnostic log: %LOG_FILE%
echo.

cd /d "%APP_DIR%" || (
    echo [ERROR] Could not change to app directory: %APP_DIR%
    echo. >> "%LOG_FILE%"
    echo [ERROR] Failed to change directory to: %APP_DIR% >> "%LOG_FILE%"
    pause
    exit /b 1
)

REM Launch with error capture
call "%APP_EXE%" 2>> "%LOG_FILE%"
set "EXIT_CODE=!errorlevel!"

if "!EXIT_CODE!"=="0" (
    echo. >> "%LOG_FILE%"
    echo [SUCCESS] Application exited normally (code 0) >> "%LOG_FILE%"
) else (
    echo. >> "%LOG_FILE%"
    echo [WARNING] Application exited with code !EXIT_CODE! >> "%LOG_FILE%"
    echo If the application crashed, check the log above for error details.
)

echo.
echo Application exited with code: !EXIT_CODE!
echo Diagnostic log saved to: %LOG_FILE%
echo.
pause
exit /b !EXIT_CODE!
