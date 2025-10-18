@echo off
echo === TA-Lib Build for AMD EPYC (Clang + OpenMP) ===
echo.

set Platform=x64

REM Clean
if exist build-epyc rmdir /s /q build-epyc

echo Configuring for AMD EPYC with OpenMP + SIMD...
cmake -B build-epyc -G "Visual Studio 17 2022" -A x64 -T ClangCL

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Configuration failed!
    echo.
    echo Please install "C++ Clang tools for Windows" in Visual Studio:
    echo   1. Open Visual Studio Installer
    echo   2. Modify VS 2022
    echo   3. Individual Components
    echo   4. Check "C++ Clang tools for Windows"
    exit /b 1
)

echo.
echo Building with OpenMP enabled...
cmake --build build-epyc --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Running regression tests...
"build-epyc\bin\Release\ta_regtest.exe"
