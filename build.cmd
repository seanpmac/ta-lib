@echo off
echo === TA-Lib Build with Clang + OpenMP ===
echo.

REM Add LLVM to PATH
set PATH=C:\Program Files\LLVM\bin;%PATH%

REM Set environment
set Platform=x64

echo Configuring with Clang...
cmake -B build-win -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-fopenmp" -DCMAKE_CXX_FLAGS="-fopenmp"

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

echo.
echo Building...
cmake --build build-win --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Running regression tests...
echo.

REM Find and run ta_regtest
for /r build-win %%f in (ta_regtest.exe) do (
    if exist "%%f" (
        echo Found: %%f
        "%%f"
        goto :done
    )
)

:done
echo.
echo Done!
