@echo off
REM Split Package Release Script for HeatXTwin_Final
echo ========================================
echo HeatXTwin Split Release Packager
echo ========================================
echo.

set VERSION=v1.0.0

REM ===== PACKAGE 1: Core Application =====
echo [Part 1: Core Application]
set CORE_DIR=releases\HeatXTwin_%VERSION%_Core
if exist "%CORE_DIR%" rmdir /s /q "%CORE_DIR%"
mkdir "%CORE_DIR%"

echo   Copying executable and DLLs...
copy /Y "build\bin\Release\HeatXTwin_Pro.exe" "%CORE_DIR%\"
copy /Y "build\bin\Release\Qt6*.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\double-conversion.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\pcre2-16.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\zlib1.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\zstd.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\brotli*.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\harfbuzz.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\freetype.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\libpng16.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\bz2.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\D3Dcompiler_47.dll" "%CORE_DIR%\"
REM Skip ICU data file for now
copy /Y "build\bin\Release\icuin74.dll" "%CORE_DIR%\"
copy /Y "build\bin\Release\icuuc74.dll" "%CORE_DIR%\"

echo   Copying plugins...
mkdir "%CORE_DIR%\platforms"
copy /Y "build\bin\Release\platforms\qwindows.dll" "%CORE_DIR%\platforms\"

mkdir "%CORE_DIR%\styles"
copy /Y "build\bin\Release\styles\qmodernwindowsstyle.dll" "%CORE_DIR%\styles\"

mkdir "%CORE_DIR%\imageformats"
xcopy /Y "build\bin\Release\imageformats\*.dll" "%CORE_DIR%\imageformats\"

mkdir "%CORE_DIR%\generic"
xcopy /Y "build\bin\Release\generic\*.dll" "%CORE_DIR%\generic\" 2>nul

echo   Copying configs and docs...
mkdir "%CORE_DIR%\configs"
xcopy /Y "configs\*.toml" "%CORE_DIR%\configs\"

echo IMPORTANT: Download BOTH files and extract to the SAME folder > "%CORE_DIR%\README_INSTALL.txt"
echo. >> "%CORE_DIR%\README_INSTALL.txt"
echo 1. Download HeatXTwin_%VERSION%_Core.zip >> "%CORE_DIR%\README_INSTALL.txt"
echo 2. Download HeatXTwin_%VERSION%_Data.zip >> "%CORE_DIR%\README_INSTALL.txt"
echo 3. Extract BOTH to the same folder >> "%CORE_DIR%\README_INSTALL.txt"
echo 4. Run HeatXTwin_Pro.exe >> "%CORE_DIR%\README_INSTALL.txt"

echo   Creating Core ZIP...
powershell -command "Compress-Archive -Path '%CORE_DIR%\*' -DestinationPath 'releases\HeatXTwin_%VERSION%_Core.zip' -CompressionLevel Optimal -Force"

REM ===== PACKAGE 2: ICU Data =====
echo.
echo [Part 2: ICU Data]
set DATA_DIR=releases\HeatXTwin_%VERSION%_Data
if exist "%DATA_DIR%" rmdir /s /q "%DATA_DIR%"
mkdir "%DATA_DIR%"

echo   Copying ICU data file...
copy /Y "build\bin\Release\icudt74.dll" "%DATA_DIR%\"

echo Extract this to the SAME folder as the Core package > "%DATA_DIR%\README_INSTALL.txt"

echo   Creating Data ZIP...
powershell -command "Compress-Archive -Path '%DATA_DIR%\*' -DestinationPath 'releases\HeatXTwin_%VERSION%_Data.zip' -CompressionLevel Optimal -Force"

REM ===== Report Sizes =====
echo.
echo ========================================
echo Package Creation Complete!
echo ========================================
echo.

for %%I in (releases\HeatXTwin_%VERSION%_Core.zip) do (
    set /a CORE_SIZE=%%~zI / 1048576
    echo Core Package: %%~nxI - !CORE_SIZE! MB
)

for %%I in (releases\HeatXTwin_%VERSION%_Data.zip) do (
    set /a DATA_SIZE=%%~zI / 1048576
    echo Data Package: %%~nxI - !DATA_SIZE! MB
)

echo.
echo Both files are under 25 MB and ready to upload!
echo.
echo Upload Instructions:
echo 1. Go to GitHub Releases
echo 2. Upload HeatXTwin_%VERSION%_Core.zip
echo 3. Upload HeatXTwin_%VERSION%_Data.zip
echo 4. Update release notes to mention BOTH files needed
echo.
pause
