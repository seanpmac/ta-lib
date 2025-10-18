@echo off
echo === TA-Lib Build with Clang + OpenMP (Fixed) ===
echo.

REM Add LLVM to PATH
set "PATH=C:\Program Files\LLVM\bin;%PATH%"

REM Clean
if exist build-openmp rmdir /s /q build-openmp

REM Set environment
set Platform=x64

echo Configuring with Clang + OpenMP...
cmake -B build-openmp -G "Ninja" ^
    -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" ^
    -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Building...
cmake --build build-openmp --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Running regression tests...
echo.

for /r build-openmp %%f in (ta_regtest.exe) do (
    if exist "%%f" (
        "%%f"
        goto :eof
    )
)
