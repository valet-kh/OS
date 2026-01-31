#!/bin/bash
echo "Pulling from git"
git pull

mkdir -p build
cd build

cmake ..
make

echo "Build complete"

