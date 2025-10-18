@echo off
echo === TA-Lib Optimized Build for AMD EPYC ===
echo.

set "PATH=C:\Program Files\LLVM\bin;%PATH%"

if exist build-epyc rmdir /s /q build-epyc

set Platform=x64

echo Configuring with Clang + OpenMP + Optimization Flags...
echo   - OpenMP enabled (SIMD + threading)
echo   - -O3 (maximum optimization)
echo   - -march=x86-64-v3 (AVX2 + FMA)
echo   - -ffast-math (aggressive FP optimization)
echo.

cmake -B build-epyc -G "Ninja" ^
    -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" ^
    -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-O3 -march=x86-64-v3 -mavx2 -mfma -fopenmp -ffast-math" ^
    -DCMAKE_CXX_FLAGS="-O3 -march=x86-64-v3 -mavx2 -mfma -fopenmp -ffast-math"

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Building with optimizations...
cmake --build build-epyc --target ta_regtest

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Running regression tests (optimized)...
echo.

for /r build-epyc %%f in (ta_regtest.exe) do (
    if exist "%%f" (
        echo Executable: %%f
        echo.
        "%%f"
        goto :eof
    )
)

echo ta_regtest.exe not found!
