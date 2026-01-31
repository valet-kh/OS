@echo off
setlocal

set PORT=COM7

cd /d "%~dp0"

if exist ../build (
    cd ../build
) else (
    echo Build folder not found
    pause
    exit /b
)

if not exist DeviceSim.exe (
    echo DeviceSim.exe not found
    pause
    exit /b
)

echo Starting DeviceSim on %PORT%...

DeviceSim.exe %PORT%

pause