@echo off
REM ===============================================
REM Windows GoogleTest Pipeline
REM ===============================================

echo Configuring CMake if not configured yet.
if not exist "build/VisualStudio" (
    CALL "Configure_VisualStudio_No_Analysis.bat" /nopause
)

echo Starting Build and Test
if exist "build/VisualStudio" (
    cd build/VisualStudio
    cmake --build . --target tester_executable --config Debug
    ctest
)

echo.
echo Test completed!
pause
