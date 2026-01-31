@echo off
setlocal

set PORT=COM8

cd /d "%~dp0.."

if exist build (
    cd build
) else (
    echo Build folder not found
    pause
    exit /b
)

if not exist LogTool.exe (
    echo LogTool.exe not found
    pause
    exit /b
)

echo Starting LogTool on %PORT%...

LogTool.exe %PORT%

pause