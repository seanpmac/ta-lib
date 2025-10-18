# TA-Lib Windows Build Environment Setup Script
# Configures and builds the optimized library with Visual Studio

Write-Host "=== TA-Lib Windows Build Environment Setup ===" -ForegroundColor Cyan
Write-Host ""

# Clean previous build
Write-Host "1. Cleaning previous build directory..." -ForegroundColor Yellow
if (Test-Path "build-win") {
    Remove-Item -Path "build-win" -Recurse -Force
    Write-Host "   Previous build-win removed" -ForegroundColor Green
}

# Set Platform environment variable (required by CMakeLists.txt check)
$env:Platform = "x64"

# Configure with CMake
Write-Host ""
Write-Host "2. Configuring with CMake (Visual Studio 2022)..." -ForegroundColor Yellow
cmake -B build-win -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: CMake configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host "   Configuration successful" -ForegroundColor Green

# Build Release configuration
Write-Host ""
Write-Host "3. Building Release configuration..." -ForegroundColor Yellow
cmake --build build-win --config Release --target ta_regtest

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host "   Build successful" -ForegroundColor Green

# Check if binaries exist
Write-Host ""
Write-Host "4. Verifying build outputs..." -ForegroundColor Yellow
$regtest = "build-win\bin\Release\ta_regtest.exe"
$perf = "build-win\bin\Release\ta_perf.exe"

if (Test-Path $regtest) {
    Write-Host "   ✓ ta_regtest.exe found" -ForegroundColor Green
} else {
    Write-Host "   ✗ ta_regtest.exe NOT found" -ForegroundColor Red
}

if (Test-Path $perf) {
    Write-Host "   ✓ ta_perf.exe found" -ForegroundColor Green
} else {
    Write-Host "   ✗ ta_perf.exe NOT found" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Setup Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run regression tests:" -ForegroundColor White
Write-Host "  cd build-win" -ForegroundColor Gray
Write-Host "  .\bin\Release\ta_regtest.exe" -ForegroundColor Gray
Write-Host ""
Write-Host "To run performance profiling:" -ForegroundColor White
Write-Host "  cd build-win" -ForegroundColor Gray
Write-Host "  .\bin\Release\ta_perf.exe" -ForegroundColor Gray
Write-Host ""
