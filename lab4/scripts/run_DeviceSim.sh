#!/bin/bash

PORT="./ttySender"

if [ -f "../build/DeviceSim" ]; then
    ../build/DeviceSim "$PORT"
else
    echo "Error: ../build/DeviceSim not found!"
fi

