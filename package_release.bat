@echo off
REM Package Release Script for HeatXTwin_Final
echo ========================================
echo HeatXTwin Release Packager
echo ========================================
echo.

REM Set version
set VERSION=v1.0.0
set PACKAGE_NAME=HeatXTwin_%VERSION%_Windows_x64

REM Create release directory
set RELEASE_DIR=releases\%PACKAGE_NAME%
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

echo [1/5] Copying executable and DLLs...
xcopy /E /I /Y "build\bin\Release\*" "%RELEASE_DIR%\"

echo [2/5] Copying configuration files...
mkdir "%RELEASE_DIR%\configs"
xcopy /Y "configs\*.toml" "%RELEASE_DIR%\configs\"

echo [3/5] Copying documentation...
copy /Y "README.md" "%RELEASE_DIR%\README.md"
copy /Y "QUICK_REFERENCE.md" "%RELEASE_DIR%\QUICK_REFERENCE.md"
copy /Y "releases\README.md" "%RELEASE_DIR%\INSTALLATION.md"

echo [4/5] Creating ZIP package...
powershell -command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath 'releases\%PACKAGE_NAME%.zip' -Force"

echo [5/5] Cleaning up...
REM Keep the directory for manual inspection
REM rmdir /s /q "%RELEASE_DIR%"

echo.
echo ========================================
echo Package created successfully!
echo Location: releases\%PACKAGE_NAME%.zip
echo ========================================
echo.
echo You can now:
echo 1. Test the package by extracting and running it
echo 2. Upload to GitHub Releases
echo 3. Share with users
echo.
pause
