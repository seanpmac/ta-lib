# TA-Lib Windows Build - Core Optimizations Without OpenMP Threading
# Tests Phase 1 optimizations: power-of-2 buffers, inline functions, etc.

Write-Host "=== TA-Lib Windows Build (Core Optimizations) ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Building with:" -ForegroundColor White
Write-Host "  - Power-of-2 circular buffers" -ForegroundColor Gray
Write-Host "  - Hilbert 64-byte circular buffers" -ForegroundColor Gray
Write-Host "  - Inline SMA helper" -ForegroundColor Gray
Write-Host "  - No OpenMP threading (ta_batch.c disabled)" -ForegroundColor Gray
Write-Host ""

# Clean previous build
Write-Host "1. Cleaning build directory..." -ForegroundColor Yellow
if (Test-Path "build-win") {
    Remove-Item -Path "build-win" -Recurse -Force
    Write-Host "   Cleaned" -ForegroundColor Green
}

# Set Platform environment variable
$env:Platform = "x64"

# Configure - explicitly disable OpenMP to avoid ta_batch.c issues
Write-Host ""
Write-Host "2. Configuring with CMake..." -ForegroundColor Yellow
cmake -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_DISABLE_FIND_PACKAGE_OpenMP=TRUE

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: Configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host "   Configuration successful" -ForegroundColor Green

# Build library and test tools
Write-Host ""
Write-Host "3. Building library and tools..." -ForegroundColor Yellow
cmake --build build-win --config Release --target ta_regtest

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host "   Build successful!" -ForegroundColor Green

# Build performance tool
Write-Host ""
Write-Host "4. Building performance profiler..." -ForegroundColor Yellow
cmake --build build-win --config Release --target ta_perf

if ($LASTEXITCODE -ne 0) {
    Write-Host "   WARNING: ta_perf build failed (may need OpenMP)" -ForegroundColor Yellow
    Write-Host "   Continuing with ta_regtest only..." -ForegroundColor Yellow
}

# Verify outputs
Write-Host ""
Write-Host "5. Verifying build outputs..." -ForegroundColor Yellow
$regtest = "build-win\bin\Release\ta_regtest.exe"
$perf = "build-win\bin\Release\ta_perf.exe"

if (Test-Path $regtest) {
    Write-Host "   ✓ ta_regtest.exe ready" -ForegroundColor Green
    $regtestExists = $true
} else {
    Write-Host "   ✗ ta_regtest.exe NOT found" -ForegroundColor Red
    $regtestExists = $false
}

if (Test-Path $perf) {
    Write-Host "   ✓ ta_perf.exe ready" -ForegroundColor Green
    $perfExists = $true
} else {
    Write-Host "   ⚠ ta_perf.exe not available" -ForegroundColor Yellow
    $perfExists = $false
}

Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Cyan
Write-Host ""

if ($regtestExists) {
    Write-Host "Ready to test and profile!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Run regression tests (validates all optimizations):" -ForegroundColor White
    Write-Host "  cd build-win" -ForegroundColor Cyan
    Write-Host "  .\bin\Release\ta_regtest.exe" -ForegroundColor Cyan
    Write-Host ""
    
    if ($perfExists) {
        Write-Host "Run performance profiling:" -ForegroundColor White
        Write-Host "  .\bin\Release\ta_perf.exe" -ForegroundColor Cyan
        Write-Host ""
    }
    
    Write-Host "Quick test now:" -ForegroundColor White
    Write-Host "  & '.\build-win\bin\Release\ta_regtest.exe'" -ForegroundColor Cyan
    Write-Host ""
}
