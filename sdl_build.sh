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

D_FLAGS="-DHANDMADE_SDL=1
         -DHANDMADE_SLOW=1
         -DHANDMADE_INTERNAL=1"

echo "Building Handmade Hero"
mkdir -p ./build/bin/
SRC_DIR="../../../src/"

pushd ./build/bin/
if ! $CC $CFLAGS $D_FLAGS -fPIC -shared -o libgame.dylib "../../src/game.c" ; then
    exit 1
fi
popd

pushd ./build/bin/
if ! $CC $CFLAGS $D_FLAGS -o "sdl_handmade" ./libgame.dylib "../../src/sdl.c" $(pkgconf --cflags --libs sdl3) ; then
    exit 1
fi
popd

# Debug / Run
if [ "${DEBUG}" ]; then
    if [ "${CC}" == "clang" ]; then
        pushd ./build/bin/
        lldb ./sdl/handmade
        popd
        exit 1
    elif [ "${CC}" == "gcc" ]; then
        if [ "$(uname)" == "Darwin" ] && [ "$(uname -p)" == "arm" ]; then
            echo gdb not supported on $(sysctl -n machdep.cpu.brand_string)
            exit 1
        fi
        pushd ./build/bin/
        gdb ./sdl/handmade
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
    ./sdl_handmade
    popd
fi
