@echo off
setlocal enabledelayedexpansion

:: Package script for foo_dsp_speedy
:: Creates .fb2k-component file (zip archive with DLLs)
:: Usage: package.bat [version]
:: Default version: 1.0.0

set VERSION=%1
if "%VERSION%"=="" set VERSION=1.0.0

set OUTPUT_DIR=release
set PACKAGE_NAME=foo_dsp_speedy-%VERSION%.fb2k-component

:: Check if builds exist
if not exist "bin\Release\Win32\foo_dsp_speedy.dll" (
    echo ERROR: Win32 Release build not found. Run build.bat first.
    exit /b 1
)
if not exist "bin\Release\x64\foo_dsp_speedy.dll" (
    echo ERROR: x64 Release build not found. Run build.bat first.
    exit /b 1
)

:: Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

:: Create temporary staging directory
set STAGE_DIR=%TEMP%\foo_dsp_speedy_package
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%"

:: Copy DLLs to staging
:: fb2k-component format: x64 DLL goes in x64 subfolder, x86 DLL in root
copy "bin\Release\Win32\foo_dsp_speedy.dll" "%STAGE_DIR%\foo_dsp_speedy.dll" >nul
mkdir "%STAGE_DIR%\x64"
copy "bin\Release\x64\foo_dsp_speedy.dll" "%STAGE_DIR%\x64\foo_dsp_speedy.dll" >nul

:: Create zip using PowerShell (create as .zip then rename to .fb2k-component)
echo Creating %PACKAGE_NAME%...
set ZIP_NAME=foo_dsp_speedy-%VERSION%.zip
if exist "%OUTPUT_DIR%\%ZIP_NAME%" del "%OUTPUT_DIR%\%ZIP_NAME%"
if exist "%OUTPUT_DIR%\%PACKAGE_NAME%" del "%OUTPUT_DIR%\%PACKAGE_NAME%"
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE_DIR%\*' -DestinationPath '%OUTPUT_DIR%\%ZIP_NAME%' -Force"

if !errorlevel! neq 0 (
    echo ERROR: Failed to create package.
    rmdir /s /q "%STAGE_DIR%"
    exit /b 1
)

:: Rename .zip to .fb2k-component
ren "%OUTPUT_DIR%\%ZIP_NAME%" "%PACKAGE_NAME%"

:: Cleanup
rmdir /s /q "%STAGE_DIR%"

echo.
echo ========================================
echo Package created successfully!
echo ========================================
echo   %OUTPUT_DIR%\%PACKAGE_NAME%
echo.
echo To install: Double-click the .fb2k-component file
echo             or drag it onto foobar2000.

exit /b 0
