@echo off
setlocal

:: Настройка порта для Логгера
set PORT=COM6

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
if not exist LogTool.exe (
    echo [ERROR] LogTool.exe not found! 
    echo Please rebuild the project using CMakeLists.txt updates.
    pause
    exit /b
)

echo.
echo [LOGGER] Starting LogTool on %PORT%...
echo Old buffer data will be flushed.
echo.

LogTool.exe %PORT%

pause