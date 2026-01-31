#!/bin/bash

PORT="./ttyReceiver"

if [ -f "../build/LogTool" ]; then
    ../build/LogTool "$PORT"
else
    echo "Error: ../build/LogTool not found!"
fi

if [ -f "web_client.html" ]; then
    cp web_client.html ../build/
fi

echo "Starting LogTool on $PORT..."
echo "Server: http://localhost:8080"

./build/LogTool "$PORT"
