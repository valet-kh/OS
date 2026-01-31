#!/bin/bash

PORT="./ttyReceiver"

if [ -f "../build/LogTool" ]; then
    ../build/LogTool "$PORT"
else
    echo "Error: ../build/LogTool not found!"
fi

