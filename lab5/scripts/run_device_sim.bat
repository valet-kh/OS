@echo off
setlocal

:: Настройка порта для Эмулятора
set PORT=COM5

:: Переходим в корень проекта
cd /d "%~dp0.."

:: Переходим в папку с бинарниками
if exist build (
    cd build
) else (
    echo [ERROR] Build folder not found! Please build the project first.
    pause
    exit /b
)

:: Проверка наличия файла
if not exist DeviceSim.exe (
    echo [ERROR] DeviceSim.exe not found!
    echo Please rebuild the project using CMakeLists.txt updates.
    pause
    exit /b
)

echo.
echo [EMULATOR] Starting DeviceSim on %PORT%...
echo Press Ctrl+C to stop.
echo.

DeviceSim.exe %PORT%

pause