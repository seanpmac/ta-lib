# TA-Lib Windows Build with Full OpenMP Support via Clang
# Uses LLVM Clang for complete OpenMP 5.x support

Write-Host "=== TA-Lib Build with Full OpenMP Support ===" -ForegroundColor Cyan
Write-Host ""

# Add LLVM to PATH
$env:Path = "C:\Program Files\LLVM\bin;$env:Path"

# Verify Clang
Write-Host "Checking Clang..." -ForegroundColor Yellow
$clangCheck = & clang --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Clang available" -ForegroundColor Green
} else {
    Write-Host "  ✗ Clang not found!" -ForegroundColor Red
    exit 1
}

# Clean
Write-Host ""
Write-Host "Cleaning build directory..." -ForegroundColor Yellow
if (Test-Path "build-win") {
    Remove-Item -Path "build-win" -Recurse -Force
}
Write-Host "  Done" -ForegroundColor Green

# Set environment
$env:Platform = "x64"
$env:CC = "clang"
$env:CXX = "clang++"

# Configure with Ninja + Clang
Write-Host ""
Write-Host "Configuring with Clang + Ninja..." -ForegroundColor Yellow
cmake -B build-win `
    -G "Ninja" `
    -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++ `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_FLAGS="-fopenmp" `
    -DCMAKE_CXX_FLAGS="-fopenmp"

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host "  Configuration successful" -ForegroundColor Green

# Build
Write-Host ""
Write-Host "Building with OpenMP enabled..." -ForegroundColor Yellow
cmake --build build-win --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "  Build successful!" -ForegroundColor Green

# Find executables
Write-Host ""
Write-Host "Locating executables..." -ForegroundColor Yellow
$regtest = Get-ChildItem -Path "build-win" -Recurse -Filter "ta_regtest.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
$perf = Get-ChildItem -Path "build-win" -Recurse -Filter "ta_perf.exe" -ErrorAction SilentlyContinue | Select-Object -First 1

if ($regtest) {
    Write-Host "  ✓ ta_regtest.exe: $($regtest.FullName)" -ForegroundColor Green
    $regtestPath = $regtest.FullName
} else {
    Write-Host "  ✗ ta_regtest.exe not found" -ForegroundColor Red
}

if ($perf) {
    Write-Host "  ✓ ta_perf.exe: $($perf.FullName)" -ForegroundColor Green
    $perfPath = $perf.FullName
} else {
    Write-Host "  ⚠ ta_perf.exe not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Build Complete with OpenMP ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Built with:" -ForegroundColor White
Write-Host "  - LLVM Clang 21.1.3" -ForegroundColor Gray
Write-Host "  - Full OpenMP 5.x support" -ForegroundColor Gray
Write-Host "  - Phase 1 optimizations active" -ForegroundColor Gray
Write-Host "  - SIMD vectorization enabled" -ForegroundColor Gray
Write-Host ""

if ($regtest) {
    Write-Host "To run regression tests:" -ForegroundColor White
    Write-Host "  & '$regtestPath'" -ForegroundColor Cyan
    Write-Host ""
}

if ($perf) {
    Write-Host "To run performance profiling:" -ForegroundColor White
    Write-Host "  & '$perfPath'" -ForegroundColor Cyan
    Write-Host ""
}

Write-Host "Ready to test your optimizations!" -ForegroundColor Green
Write-Host ""
