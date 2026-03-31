@echo off
REM ===============================================
REM Windows GoogleTest Pipeline
REM ===============================================

echo Configuring CMake if not configured yet.
CALL "Configure_VisualStudio_No_Analysis.bat" /nopause
if errorlevel 1 (
    echo Configuration failed!
    exit /b 1
)


echo Starting Build and Test (Debug)
if exist "build/VisualStudio" (
    cd build/VisualStudio
    cmake --build . --target tester_executable --config Debug
    if errorlevel 1 (
        echo Tester failed to build!
        exit /b 1
    )
    ctest -C Debug
    if errorlevel 1 (
        echo Test failed!
        exit /b 1
    )
)

echo Starting Build and Test (Release)
if exist "build/VisualStudio" (
    cmake --build . --target tester_executable --config Release
    if errorlevel 1 (
        echo Tester failed to build!
        exit /b 1
    )
    ctest -C Release
    if errorlevel 1 (
        echo Test failed!
        exit /b 1
    )
)

echo.
echo Test completed!

exit /b 0
