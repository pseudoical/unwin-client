#!/bin/bash

set -eux

chmod 4755 ./cef/Release/chrome-sandbox

gcc \
    -std=c23 \
    -Wall -Wextra -Wpedantic \
    $(find ./src -type f -name "*.c") \
    -I./cef/ -I./include/ \
    -L./cef/Release \
    -Wl,-rpath,'$ORIGIN/cef/Release' \
    -lcef -lX11 \
    -pthread \
    -O2 -o ./unwin
