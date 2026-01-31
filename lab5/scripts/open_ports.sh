#!/bin/bash

cd "$(dirname "$0")"

socat -d -d pty,raw,echo=0,link=./ttySender pty,raw,echo=0,link=./ttyReceiver
