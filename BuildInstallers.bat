@echo off
setlocal enabledelayedexpansion

:: --- Setup Paths ---
set "InstallerDir=%~dp0Installer"
set "InstallerBuildDir=%InstallerDir%\INSTALLER"
set "InstallerGameDir=%InstallerDir%\GAMEDIRECTORY"
set "WindowsBuildDir=%~dp0build\VisualStudio\Release"
set "AndroidDir=%~dp0Android"
set "Androidx86_64BuildDir=%AndroidDir%\app\build\outputs\apk\x86_64\release"
set "AndroidArm64BuildDir=%AndroidDir%\app\build\outputs\apk\arm64\release"

echo ===============================================
echo Creating Windows Installer
echo ===============================================

echo Configuring CMake
CALL "Configure_VisualStudio_No_Analysis.bat" /nopause
if errorlevel 1 (
    echo Configuration failed!
    exit /b 1
)

echo Starting Release Build
cd build/VisualStudio
cmake --build . --target application_executable --config Release
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)
cd ../..

echo Copying Build Files to Installer

:: 1. Clean the directory
if exist "%InstallerGameDir%" rmdir /s /q "%InstallerGameDir%"
mkdir "%InstallerGameDir%"

:: 2. Copy contents from Release to GAMEDIRECTORY
:: /e = recursive including empty folders
robocopy "%WindowsBuildDir%" "%InstallerGameDir%" /e

:: 3. Copy the Extern folder into GAMEDIRECTORY
robocopy ".\Extern" "%InstallerGameDir%\Extern" /e

REM --- Find iscc.exe ---
where iscc.exe >nul 2>&1
if !errorlevel! equ 0 (
    set "ISCC=iscc.exe"
) else if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
    set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
) else if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
    set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
) else (
    echo ERROR: Inno Setup not found.
    exit /b 1
)
echo Using Inno Setup: %ISCC%

REM --- Build Installer ---
echo.
echo === Building Installer ===
"%ISCC%" "%InstallerDir%\InstallScript.iss"
if !errorlevel! neq 0 (
    echo ERROR: Inno Setup compilation failed.
    exit /b 1
)

echo Windows Installer placed in Installer/INSTALLER

echo ===============================================
echo Creating Android APKs
echo ===============================================

echo ========================================
echo Starting Android x86_64 + arm64 Builds
echo ========================================

cd /d "%AndroidDir%"

:: Clean and Build both architectures
call gradlew.bat clean assembleX86_64Release assembleArm64Release --parallel --stacktrace
if errorlevel 1 (
    echo Android build failed
    cd ..
    exit /b 1
)

cd ..

echo ===============================================
echo Copying Android APKs to Installer Directory
echo ===============================================

:: Ensure destination exists
if not exist "%InstallerBuildDir%" mkdir "%InstallerBuildDir%"

:: Copy x86_64 APKs
if exist "%Androidx86_64BuildDir%" (
    robocopy "%Androidx86_64BuildDir%" "%InstallerBuildDir%" *.apk /ndl /njh /njs /nc /ns
    echo Copied x86_64 APKs
) else (
    echo WARNING: x86_64 build directory not found.
)

:: Copy Arm64 APKs
if exist "%AndroidArm64BuildDir%" (
    robocopy "%AndroidArm64BuildDir%" "%InstallerBuildDir%" *.apk /ndl /njh /njs /nc /ns
    echo Copied Arm64 APKs
) else (
    echo WARNING: Arm64 build directory not found.
)

echo.
echo All builds complete. Check %InstallerBuildDir% for Windows and Android outputs.
exit /b 0