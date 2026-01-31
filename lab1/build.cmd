git pull

if exist build (
    rmdir /s /q build
)
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

echo Build complete! Executable is in the .\build directory
pause