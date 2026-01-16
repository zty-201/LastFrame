@echo off
REM ===============================================
REM Windows GoogleTest Pipeline
REM ===============================================

echo Configuring CMake if not configured yet.
if not exist "build/VisualStudio" (
    CALL "Configure_VisualStudio_No_Analysis.bat" /nopause
    if errorlevel 1 (
        echo Configuration failed!
        exit /b 1
    )
)

echo Starting Build and Test
if exist "build/VisualStudio" (
    cd build/VisualStudio
    cmake --build . --target tester_executable --config Debug
    if errorlevel 1 (
        echo Tester failed to build!
        exit /b 1
    )
    ctest
    if errorlevel 1 (
        echo Test failed!
        exit /b 1
    )
)

echo.
echo Test completed!
pause
exit /b 0