@echo off
setlocal enabledelayedexpansion

:: Build script for foo_dsp_speedy
:: Usage: build.bat [x86|x64|all] [Debug|Release]
:: Default: build.bat all Release

set PLATFORM=%1
set CONFIG=%2

if "%PLATFORM%"=="" set PLATFORM=all
if "%CONFIG%"=="" set CONFIG=Release

:: Find MSBuild
set MSBUILD=
for %%p in (
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) do (
    if exist %%p (
        set MSBUILD=%%p
        goto :found_msbuild
    )
)

echo ERROR: MSBuild not found. Please install Visual Studio 2019 or 2022.
exit /b 1

:found_msbuild
echo Using MSBuild: %MSBUILD%

:: Check if dependencies exist
if not exist "lib\foobar2000_SDK" (
    echo ERROR: foobar2000 SDK not found. Please run setup_deps.bat first.
    exit /b 1
)

:: Build
set BUILD_FAILED=0

if "%PLATFORM%"=="x86" goto :build_x86
if "%PLATFORM%"=="Win32" goto :build_x86
if "%PLATFORM%"=="x64" goto :build_x64
if "%PLATFORM%"=="all" goto :build_all

echo ERROR: Invalid platform "%PLATFORM%". Use x86, x64, or all.
exit /b 1

:build_all
call :build_x86
if !errorlevel! neq 0 set BUILD_FAILED=1
call :build_x64
if !errorlevel! neq 0 set BUILD_FAILED=1
goto :done

:build_x86
echo.
echo ========================================
echo Building %CONFIG%^|Win32...
echo ========================================
%MSBUILD% foo_dsp_speedy.sln -p:Configuration=%CONFIG% -p:Platform=Win32 -p:PlatformToolset=v143 -verbosity:minimal -m
if !errorlevel! neq 0 (
    echo ERROR: Win32 build failed!
    exit /b 1
)
echo Win32 build successful!
goto :eof

:build_x64
echo.
echo ========================================
echo Building %CONFIG%^|x64...
echo ========================================
%MSBUILD% foo_dsp_speedy.sln -p:Configuration=%CONFIG% -p:Platform=x64 -p:PlatformToolset=v143 -verbosity:minimal -m
if !errorlevel! neq 0 (
    echo ERROR: x64 build failed!
    exit /b 1
)
echo x64 build successful!
goto :eof

:done
if %BUILD_FAILED%==1 (
    echo.
    echo Build completed with errors.
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo Output files:
if "%PLATFORM%"=="x86" (
    echo   bin\%CONFIG%\Win32\foo_dsp_speedy.dll
) else if "%PLATFORM%"=="x64" (
    echo   bin\%CONFIG%\x64\foo_dsp_speedy.dll
) else (
    echo   bin\%CONFIG%\Win32\foo_dsp_speedy.dll
    echo   bin\%CONFIG%\x64\foo_dsp_speedy.dll
)
echo.
echo To create fb2k-component package, run: package.bat

exit /b 0
