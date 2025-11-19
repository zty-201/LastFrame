@echo off
REM ===============================================
REM Windows Clean Script
REM ===============================================

echo.
echo ===============================================
echo   Cleaning Build Directory - Windows
echo ===============================================
echo.

echo Deleting windows build folder, android build/cxx/extern folder (since they're all temporary).
if exist "build" (
    echo Removing build...
    rmdir /s /q "build"
)

if exist "Android\build" (
    echo Removing Android\build...
    rmdir /s /q "Android\build"
)

if exist "Android\app\build" (
    echo Removing Android\app\build...
    rmdir /s /q "Android\app\build"
)

if exist "Android\app\.cxx" (
    echo Removing Android\app\.cxx...
    rmdir /s /q "Android\app\.cxx"
)

echo.
echo Clean completed!
pause
