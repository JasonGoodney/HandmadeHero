#!/bin/bash

# Flags
CLEAN=''
DEBUG=''
GUI_DEBUG=''
RUN=''

while getopts 'cdgr' flag; do
    case "${flag}" in
    c) CLEAN='true' ;;
    d) DEBUG='true' ;;
    g) GUI_DEBUG='true' ;;
    r) RUN='true' ;;
    esac
done

# Clean
if [ "${CLEAN}" ]; then
    echo "Removing /build"
    rm -rf ./build
    if [ ! "${RUN}" ] && [ ! "${DEBUG}" ]; then
        exit 1
    fi
fi

# Build
CXX="clang"

CXX_FLAGS="-g
           -Werror
           -Wall"

OSX_LD_FLAGS="-framework AppKit
              -framework IOKit
              -framework AudioToolbox"

D_FLAGS="-DHANDMADE_MAC=1
         -DHANDMADE_SLOW=1
         -DHANDMADE_INTERNAL=1"

echo "Building Handmade Hero"
mkdir -p ./build/bin/macos
pushd ./build/bin/macos
if ! $CXX $CXX_FLAGS $D_FLAGS $OSX_LD_FLAGS -o handmade ../../../src/macos/macos_main.m; then
    exit 1
fi
popd

# Debug / Run
if [ "${DEBUG}" ]; then
    if [ "${CXX}" == "clang" ]; then
        lldb ./build/bin/macos/handmade
        exit 1
    elif [ "${CXX}" == "gcc" ]; then
        if [ "$(uname)" == "Darwin" ] && [ "$(uname -p)" == "arm" ]; then
            echo gdb not supported on $(sysctl -n machdep.cpu.brand_string)
            exit 1
        fi
        gdb ./build/bin/macos/handmade
        exit 1
    fi
fi

if [ "${GUI_DEBUG}" ]; then
    open -a Xcode ./debug/macos_debug/macos_debug.xcodeproj
    exit 1
fi

if [ "${RUN}" ]; then
    echo "Running Handmade Hero"
    ./build/bin/macos/handmade
fi
