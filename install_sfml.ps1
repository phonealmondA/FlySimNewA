# SFML Installation Helper for Windows
# Run with: powershell -ExecutionPolicy Bypass -File install_sfml.ps1

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "  SFML Installation Helper (Windows)" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

# Check if vcpkg is available
$vcpkgPath = $null
if (Get-Command vcpkg -ErrorAction SilentlyContinue) {
    $vcpkgPath = (Get-Command vcpkg).Source
    Write-Host "Found vcpkg at: $vcpkgPath" -ForegroundColor Green
}

if ($vcpkgPath) {
    Write-Host ""
    Write-Host "Installing SFML via vcpkg..." -ForegroundColor Yellow
    Write-Host "This may take several minutes..." -ForegroundColor Yellow

    vcpkg install sfml:x64-windows csfml:x64-windows

    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ SFML installed successfully!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Setting environment variables..." -ForegroundColor Yellow

        # Get vcpkg root
        $vcpkgRoot = Split-Path -Parent $vcpkgPath
        $sfmlPath = Join-Path $vcpkgRoot "installed\x64-windows"

        Write-Host "SFML installed to: $sfmlPath" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Add this to your environment variables:" -ForegroundColor Yellow
        Write-Host "  SFML_DIR=$sfmlPath" -ForegroundColor White
    } else {
        Write-Host ""
        Write-Host "✗ Installation failed" -ForegroundColor Red
    }
} else {
    Write-Host "vcpkg not found. You have two options:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "OPTION 1: Install via vcpkg (Recommended)" -ForegroundColor Cyan
    Write-Host "  1. Install vcpkg from: https://vcpkg.io/en/getting-started.html" -ForegroundColor White
    Write-Host "  2. Run: vcpkg install sfml:x64-windows csfml:x64-windows" -ForegroundColor White
    Write-Host ""
    Write-Host "OPTION 2: Manual Installation" -ForegroundColor Cyan
    Write-Host "  1. Download SFML from: https://www.sfml-dev.org/download.php" -ForegroundColor White
    Write-Host "  2. Download CSFML from: https://www.sfml-dev.org/download/csfml/" -ForegroundColor White
    Write-Host "  3. Extract and set SFML_DIR environment variable" -ForegroundColor White
    Write-Host ""
    Write-Host "OPTION 3: Use the pre-built binaries (Easiest)" -ForegroundColor Cyan
    Write-Host "  Opening SFML download page in browser..." -ForegroundColor White
    Start-Process "https://www.sfml-dev.org/download.php"
}

Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "After SFML is installed, run:" -ForegroundColor Yellow
Write-Host "  run.cmd" -ForegroundColor White
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
