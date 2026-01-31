if exist build (
    rmdir /s /q build
)
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

echo Build complete
pause