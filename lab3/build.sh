#!/bin/bash

mkdir -p build
cd build

cmake ..
make

echo "Build complete"
