@echo off
echo === TA-Lib Build with OpenMP (Library + Regtest Only) ===
echo.

REM Add LLVM to PATH  
set "PATH=C:\Program Files\LLVM\bin;%PATH%"

REM Clean
if exist build-openmp rmdir /s /q build-openmp

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
echo Building library and regression tests...
cmake --build build-openmp --target ta_regtest

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

echo ta_regtest.exe not found!
