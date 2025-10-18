# TA-Lib Windows Build with Clang/LLVM (Full OpenMP Support)
# Uses Clang which has complete OpenMP 5.x support

Write-Host "=== TA-Lib Windows Build with Clang/LLVM ===" -ForegroundColor Cyan
Write-Host ""

# Add LLVM to PATH for this session
$llvmPath = "C:\Program Files\LLVM\bin"
if (Test-Path $llvmPath) {
    $env:Path = "$llvmPath;$env:Path"
    Write-Host "Added LLVM to PATH: $llvmPath" -ForegroundColor Green
} else {
    Write-Host "ERROR: LLVM not found at $llvmPath" -ForegroundColor Red
    Write-Host "Please verify LLVM installation" -ForegroundColor Yellow
    exit 1
}

# Verify Clang is available
Write-Host ""
Write-Host "Verifying Clang installation..." -ForegroundColor Yellow
$clangVersion = & clang --version 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Clang found:" -ForegroundColor Green
    Write-Host "    $($clangVersion[0])" -ForegroundColor Gray
} else {
    Write-Host "  ✗ Clang not found!" -ForegroundColor Red
    exit 1
}

# Clean previous build
Write-Host ""
Write-Host "Cleaning previous build directory..." -ForegroundColor Yellow
if (Test-Path "build-clang") {
    Remove-Item -Path "build-clang" -Recurse -Force
    Write-Host "  Previous build-clang removed" -ForegroundColor Green
}

# Set Platform environment variable
$env:Platform = "x64"

# Configure with CMake using Clang
Write-Host ""
Write-Host "Configuring with CMake (Clang compiler)..." -ForegroundColor Yellow
cmake -B build-clang `
    -G "Ninja" `
    -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++ `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_MAKE_PROGRAM=ninja

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "CMake configuration failed. Trying with Visual Studio generator..." -ForegroundColor Yellow
    Write-Host ""
    
    # Fallback to Visual Studio generator with LLVM toolset
    cmake -B build-clang `
        -G "Visual Studio 17 2022" `
        -A x64 `
        -T ClangCL `
        -DCMAKE_BUILD_TYPE=Release
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  ERROR: CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "  Configuration successful" -ForegroundColor Green

# Build
Write-Host ""
Write-Host "Building with Clang..." -ForegroundColor Yellow
cmake --build build-clang --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "  Build successful!" -ForegroundColor Green

# Verify outputs
Write-Host ""
Write-Host "Verifying build outputs..." -ForegroundColor Yellow

# Check for executables in different possible locations
$possiblePaths = @(
    "build-clang\bin\Release\ta_regtest.exe",
    "build-clang\bin\ta_regtest.exe",
    "build-clang\Release\bin\ta_regtest.exe",
    "build-clang\ta_regtest.exe"
)

$regtestFound = $false
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        Write-Host "  ✓ ta_regtest.exe found at: $path" -ForegroundColor Green
        $regtestPath = $path
        $regtestFound = $true
        break
    }
}

if (-not $regtestFound) {
    Write-Host "  ✗ ta_regtest.exe NOT found" -ForegroundColor Red
    Write-Host "  Searched locations:" -ForegroundColor Yellow
    foreach ($path in $possiblePaths) {
        Write-Host "    - $path" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Built with:" -ForegroundColor White
Write-Host "  - Clang/LLVM 21.1.3" -ForegroundColor Gray
Write-Host "  - Full OpenMP 5.x support" -ForegroundColor Gray
Write-Host "  - Phase 1 optimizations active" -ForegroundColor Gray
Write-Host ""

if ($regtestFound) {
    Write-Host "To run regression tests:" -ForegroundColor White
    Write-Host "  $regtestPath" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Quick test command:" -ForegroundColor White
    Write-Host "  & '$regtestPath'" -ForegroundColor Cyan
}

Write-Host ""
