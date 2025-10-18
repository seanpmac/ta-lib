@echo off
echo === TA-Lib Optimized Build for Intel i9-9900K ===
echo.
echo Current Hardware:
echo   CPU: Intel i9-9900K (8 cores / 16 threads)
echo   Architecture: Coffee Lake
echo   SIMD: AVX2 (256-bit)
echo.

set "PATH=C:\Program Files\LLVM\bin;%PATH%"

if exist build-i9 rmdir /s /q build-i9

set Platform=x64

echo Build Configuration:
echo   - Compiler: Clang with OpenMP
echo   - Optimization: -O3 (aggressive)
echo   - Target: -march=skylake (Coffee Lake = Skylake derivative)
echo   - SIMD: AVX2 + FMA
echo   - Threading: OpenMP (for 16 threads)
echo.

cmake -B build-i9 -G "Ninja" ^
    -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" ^
    -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-O3 -march=skylake -mavx2 -mfma -fopenmp -ffast-math" ^
    -DCMAKE_CXX_FLAGS="-O3 -march=skylake -mavx2 -mfma -fopenmp -ffast-math"

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Building...
cmake --build build-i9 --target ta_regtest

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Performance Comparison (Same PC):
echo   - build-win (MSVC, no OpenMP):    1,789 ms
echo   - build-openmp (Clang, no opts): 4,033 ms
echo   - build-i9 (Clang, optimized):   ? ms (testing now)
echo.
echo Running regression tests...
echo.

for /r build-i9 %%f in (ta_regtest.exe) do (
    if exist "%%f" (
        "%%f"
        goto :eof
    )
)
