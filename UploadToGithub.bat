@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "InstallerBuildDir=%~dp0Installer\INSTALLER"
set "RepoURL=https://github.com/JoelLeeJie/LastFrame-Cheerios.git"

:: Use Jenkins Build Number for versioning, fallback to timestamp if running locally
if "%BUILD_NUMBER%"=="" (
    set "VERSION=v1.0.%date:~10,4%%date:~4,2%%date:~7,2%-%time:~0,2%%time:~3,2%"
    set "VERSION=%VERSION: =0%"
) else (
    set "VERSION=build-%BUILD_NUMBER%"
)

echo ===============================================
echo Preparing to upload artifacts to GitHub
echo Version: %VERSION%
echo ===============================================

:: 1. Verify Directory
if not exist "%InstallerBuildDir%" (
    echo ERROR: Build directory %InstallerBuildDir% not found!
    exit /b 1
)

:: 2. Check for GitHub CLI (gh.exe)
where gh >nul 2>&1
if %errorlevel% equ 0 (
    echo Using GitHub CLI to create a Release...
    
    :: Create a release and upload all .exe and .apk files found in the folder
    :: Note: Jenkins credentials usually provide GITHUB_TOKEN automatically.
    gh release create %VERSION% "%InstallerBuildDir%\*.exe" "%InstallerBuildDir%\*.apk" --repo JoelLeeJie/LastFrame-Cheerios --title "Release %VERSION%" --notes "Automated build from Jenkins. Commit: %GIT_COMMIT%"
    
    if !errorlevel! equ 0 (
        echo Successfully created GitHub Release and uploaded files.
        exit /b 0
    ) else (
        echo GitHub CLI upload failed. Trying fallback...
    )
)


exit /b 0