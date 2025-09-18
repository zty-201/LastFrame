@echo off
REM ===============================================
REM Windows Build and Run Script
REM ===============================================

echo.
echo ===============================================
echo   Minimal C++ Console Application - Windows
echo ===============================================
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check if Visual Studio Build Tools are available
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Visual Studio Build Tools not found in PATH
    echo Make sure you have Visual Studio or Build Tools installed
    echo.
)

REM Clean previous build
echo [1/4] Cleaning previous build...
if exist build (
    rmdir /s /q build
    echo    Previous build directory removed
) else (
    echo    No previous build found
)

REM Create build directory
echo [2/4] Creating build directory...
mkdir build

REM Used to reconfigure via indicating the cmakelists file has changed. 
copy /b "CMakeLists.txt" +,,


REM Configure with CMake
echo [3/4] Configuring project with CMake...
cmake --preset visual_studio_x64_no_analyze
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)

echo.
echo ===============================================
echo   Configuration completed successfully!
echo ===============================================
echo.

echo.
echo Press any key to exit...
pause >nul
