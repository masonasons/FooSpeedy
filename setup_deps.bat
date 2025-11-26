@echo off
setlocal

:: Setup dependencies for foo_dsp_speedy
:: This script clones all required libraries

echo Setting up dependencies for foo_dsp_speedy...
echo.

:: Create lib directory
if not exist "lib" mkdir "lib"
cd lib

:: Clone foobar2000 SDK
if not exist "foobar2000_SDK" (
    echo Cloning foobar2000 SDK...
    git clone https://github.com/reupen/foobar2000-sdk-unmodified.git foobar2000_SDK
    if !errorlevel! neq 0 (
        echo ERROR: Failed to clone foobar2000 SDK
        exit /b 1
    )
) else (
    echo foobar2000 SDK already exists, skipping...
)

:: Clone Sonic library
if not exist "sonic_repo" (
    echo Cloning Sonic library...
    git clone https://github.com/waywardgeek/sonic.git sonic_repo
    if !errorlevel! neq 0 (
        echo ERROR: Failed to clone Sonic library
        exit /b 1
    )
) else (
    echo Sonic library already exists, skipping...
)

:: Clone Speedy library (Google's fork)
if not exist "speedy_repo" (
    echo Cloning Speedy library...
    git clone https://github.com/google/speedy.git speedy_repo
    if !errorlevel! neq 0 (
        echo ERROR: Failed to clone Speedy library
        exit /b 1
    )
) else (
    echo Speedy library already exists, skipping...
)

:: Clone KISS FFT
if not exist "kissfft" (
    echo Cloning KISS FFT...
    git clone https://github.com/mborgerding/kissfft.git kissfft
    if !errorlevel! neq 0 (
        echo ERROR: Failed to clone KISS FFT
        exit /b 1
    )
) else (
    echo KISS FFT already exists, skipping...
)

cd ..

echo.
echo ========================================
echo Dependencies setup complete!
echo ========================================
echo.
echo You can now build the project with: build.bat

exit /b 0
