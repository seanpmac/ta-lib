# TA-Lib Windows Build with Clang via Visual Studio (ClangCL)
# Uses Visual Studio generator with Clang toolset for better OpenMP support

Write-Host "=== TA-Lib Windows Build with Clang (Visual Studio) ===" -ForegroundColor Cyan
Write-Host ""

# Clean previous build
Write-Host "1. Cleaning previous build directory..." -ForegroundColor Yellow
if (Test-Path "build-clang") {
    Remove-Item -Path "build-clang" -Recurse -Force
    Write-Host "   Previous build-clang removed" -ForegroundColor Green
}

# Set Platform environment variable
$env:Platform = "x64"

# Configure with CMake using Visual Studio + ClangCL toolset
Write-Host ""
Write-Host "2. Configuring with CMake (Visual Studio 2022 + ClangCL)..." -ForegroundColor Yellow
cmake -B build-clang `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -T ClangCL `
    -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host "   Configuration successful" -ForegroundColor Green

# Build
Write-Host ""
Write-Host "3. Building with ClangCL..." -ForegroundColor Yellow
cmake --build build-clang --config Release --target ta_regtest

if ($LASTEXITCODE -ne 0) {
    Write-Host "   ERROR: Build failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "   Note: If errors persist, the issue may be with ta_batch.c" -ForegroundColor Yellow
    Write-Host "   OpenMP threading code. Core optimizations don't require it." -ForegroundColor Yellow
    exit 1
}

Write-Host "   Build successful!" -ForegroundColor Green

# Verify outputs
Write-Host ""
Write-Host "4. Verifying build outputs..." -ForegroundColor Yellow
$regtest = "build-clang\bin\Release\ta_regtest.exe"

if (Test-Path $regtest) {
    Write-Host "   ✓ ta_regtest.exe found" -ForegroundColor Green
} else {
    Write-Host "   ✗ ta_regtest.exe NOT found" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run regression tests:" -ForegroundColor White
Write-Host "  cd build-clang" -ForegroundColor Gray
Write-Host "  .\bin\Release\ta_regtest.exe" -ForegroundColor Gray
Write-Host ""
Write-Host "Or quick test:" -ForegroundColor White
Write-Host "  & '.\build-clang\bin\Release\ta_regtest.exe'" -ForegroundColor Cyan
Write-Host ""
