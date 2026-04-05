@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
:: This assumes the script is running from the root of the repo
set "InstallerBuildDir=Installer\INSTALLER"

:: --- Authentication Check (CRITICAL) ---
if "%GH_TOKEN%"=="" (
    echo ERROR: GH_TOKEN is not set. 
    echo Go to Jenkins > Job > Build Environment > Use secret text and set GH_TOKEN.
    exit /b 1
)

:: Use Jenkins Build Number for versioning
if "%BUILD_NUMBER%"=="" (
    set "VERSION=v1.0.local"
) else (
    set "VERSION=build-%BUILD_NUMBER%"
)

echo Target Directory: %InstallerBuildDir%

:: 1. Verify Directory
if not exist "%InstallerBuildDir%" (
    echo ERROR: Build directory %InstallerBuildDir% not found!
    dir /s /b  :: This helps you see where you actually are in the Jenkins log
    exit /b 1
)

:: 2. Use GitHub CLI
where gh >nul 2>&1
if %errorlevel% equ 0 (
    echo Creating Release %VERSION%...
    
    :: Use pushd to enter the folder safely
    pushd "%InstallerBuildDir%"
    
    gh release create %VERSION% *.exe *.apk --repo JoelLeeJie/LastFrame-Cheerios --title "Release %VERSION%" --notes "Automated build"
    
    set "EXIT_CODE=!errorlevel!"
    popd
    exit /b !EXIT_CODE!
) else (
    echo GitHub CLI not found.
    exit /b 1
)