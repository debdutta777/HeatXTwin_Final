@echo off
REM Optimized Package Release Script for HeatXTwin_Final
echo ========================================
echo HeatXTwin Release Packager (Optimized)
echo ========================================
echo.

REM Set version
set VERSION=v1.0.0
set PACKAGE_NAME=HeatXTwin_%VERSION%_Windows_x64_Lite

REM Create release directory
set RELEASE_DIR=releases\%PACKAGE_NAME%
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

echo [1/6] Copying essential executable and core DLLs...
REM Copy only the executable
copy /Y "build\bin\Release\HeatXTwin_Pro.exe" "%RELEASE_DIR%\"

REM Copy essential Qt DLLs
copy /Y "build\bin\Release\Qt6Charts.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6Core.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6Gui.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6Widgets.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6OpenGL.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6OpenGLWidgets.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\Qt6Network.dll" "%RELEASE_DIR%\"

REM Copy essential support DLLs
copy /Y "build\bin\Release\icudt74.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\icuin74.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\icuuc74.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\double-conversion.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\pcre2-16.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\zlib1.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\zstd.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\brotlicommon.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\brotlidec.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\harfbuzz.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\freetype.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\libpng16.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\bz2.dll" "%RELEASE_DIR%\"
copy /Y "build\bin\Release\D3Dcompiler_47.dll" "%RELEASE_DIR%\"

echo [2/6] Copying plugin directories...
mkdir "%RELEASE_DIR%\platforms"
copy /Y "build\bin\Release\platforms\qwindows.dll" "%RELEASE_DIR%\platforms\"

mkdir "%RELEASE_DIR%\styles"
copy /Y "build\bin\Release\styles\qmodernwindowsstyle.dll" "%RELEASE_DIR%\styles\"

mkdir "%RELEASE_DIR%\imageformats"
copy /Y "build\bin\Release\imageformats\qico.dll" "%RELEASE_DIR%\imageformats\"
copy /Y "build\bin\Release\imageformats\qjpeg.dll" "%RELEASE_DIR%\imageformats\"

REM Skip optional plugins to save space
REM mkdir "%RELEASE_DIR%\generic"
REM mkdir "%RELEASE_DIR%\networkinformation"
REM mkdir "%RELEASE_DIR%\tls"

echo [3/6] Copying configuration files...
mkdir "%RELEASE_DIR%\configs"
xcopy /Y "configs\*.toml" "%RELEASE_DIR%\configs\"

echo [4/6] Copying minimal documentation...
copy /Y "releases\README.md" "%RELEASE_DIR%\INSTALLATION.md"

echo [5/6] Creating compressed ZIP package...
powershell -command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath 'releases\%PACKAGE_NAME%.zip' -CompressionLevel Optimal -Force"

echo [6/6] Checking package size...
for %%I in (releases\%PACKAGE_NAME%.zip) do set SIZE=%%~zI
set /a SIZE_MB=%SIZE% / 1048576

echo.
echo ========================================
echo Package created successfully!
echo Location: releases\%PACKAGE_NAME%.zip
echo Size: %SIZE_MB% MB
echo ========================================
echo.

if %SIZE_MB% GTR 25 (
    echo WARNING: Package is larger than 25 MB!
    echo GitHub releases limit is 25 MB per file.
    echo.
    echo SOLUTIONS:
    echo 1. Use GitHub Releases API ^(allows up to 2GB^)
    echo 2. Use UPX to compress the executable
    echo 3. Host on external service ^(Google Drive, Dropbox^)
    echo 4. Use Git LFS for large files
) else (
    echo SUCCESS: Package is under 25 MB and ready to upload!
)

echo.
pause
