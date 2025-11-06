@echo off
echo ====================================
echo   KatieFlySimRust Launcher
echo   Pure Rust - Zero Dependencies!
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
    echo Please check the error messages above.
    echo.
    pause
    exit /b 1
)

echo.
echo Game closed successfully!
pause
