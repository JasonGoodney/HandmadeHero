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
    rm -rf *.dylib
    rm -rf *.dSYM
    if [ ! "${RUN}" ] && [ ! "${DEBUG}" ]; then
        exit 1
    fi
fi

# Build
CC="clang"

CFLAGS="-g
        -std=c99
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-null-dereference"

OSX_LD_FLAGS="-framework AppKit
              -framework IOKit
              -framework AudioToolbox"

D_FLAGS="-DHANDMADE_MAC=1
         -DHANDMADE_SLOW=1
         -DHANDMADE_INTERNAL=1"

echo "Building Handmade Hero"
mkdir -p ./build/bin/macos
SRC_DIR="../../../src/"

pushd ./build/bin/
if ! $CC $CFLAGS $D_FLAGS $OSX_LD_FLAGS -fPIC -shared -o libgame.dylib "../../src/game.c" ; then
    exit 1
fi
popd

pushd ./build/bin/macos
if ! $CC $CFLAGS $D_FLAGS $OSX_LD_FLAGS -o handmade ../libgame.dylib "../../../src/macos/macos_main.m" ; then
    exit 1
fi
popd

# Debug / Run
if [ "${DEBUG}" ]; then
    if [ "${CC}" == "clang" ]; then
        pushd ./build/bin/
        lldb ./macos/handmade
        popd
        exit 1
    elif [ "${CC}" == "gcc" ]; then
        if [ "$(uname)" == "Darwin" ] && [ "$(uname -p)" == "arm" ]; then
            echo gdb not supported on $(sysctl -n machdep.cpu.brand_string)
            exit 1
        fi
        pushd ./build/bin/
        gdb ./macos/handmade
        popd
        exit 1
    fi
fi

if [ "${GUI_DEBUG}" ]; then
    open -a Xcode ./debug/macos_debug/macos_debug.xcodeproj
    exit 1
fi

if [ "${RUN}" ]; then
    echo "Running Handmade Hero"
    pushd ./build/bin/
    ./macos/handmade
    popd
fi
