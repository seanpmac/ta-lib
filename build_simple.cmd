@echo off
echo === TA-Lib Build (Core Optimizations) ===
echo.
echo Your Phase 1 optimizations will be active:
echo   - Power-of-2 circular buffers
echo   - Hilbert 64-byte buffers  
echo   - Inline SMA helper
echo.

set Platform=x64

echo Configuring...
cmake -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_DISABLE_FIND_PACKAGE_OpenMP=TRUE

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Building library and tests...
cmake --build build-win --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Running regression tests to validate optimizations...
echo.

if exist "build-win\bin\Release\ta_regtest.exe" (
    "build-win\bin\Release\ta_regtest.exe"
) else (
    echo ta_regtest.exe not found!
    exit /b 1
)
