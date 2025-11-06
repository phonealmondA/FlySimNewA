@echo off
echo ====================================
echo   KatieFlySimRust Launcher
echo ====================================
echo.

REM Check if Rust is installed
where cargo >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Cargo not found!
    echo.
    echo Please install Rust from: https://rustup.rs/
    echo.
    pause
    exit /b 1
)

echo Rust/Cargo found!
echo.

REM Check if SFML is available (just a warning, not blocking)
echo NOTE: This game requires SFML libraries to be installed.
echo If compilation fails, install SFML from: https://www.sfml-dev.org/
echo.

REM Navigate to Rust project directory
cd KatieFlySimRust

echo Building and running KatieFlySimRust...
echo This may take a few minutes on first run...
echo.

REM Build and run in release mode for better performance
cargo run --release

REM Check if cargo run succeeded
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ====================================
    echo   Build/Run Failed!
    echo ====================================
    echo.
    echo Common issues:
    echo 1. SFML not installed - Install from https://www.sfml-dev.org/
    echo 2. Missing Visual Studio Build Tools - Install from:
    echo    https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

echo.
echo Game closed successfully!
pause
