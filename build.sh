#!/bin/bash

CXX="clang"

CXX_FLAGS="-g -Wall"

OSX_LD_FLAGS="-framework AppKit 
              -framework IOKit"

EXECUTABLE="handmade"

# Flags
CLEAN=''
DEBUG=''
RUN=''

while getopts 'cd:r' flag; do
    case "${flag}" in
    c) CLEAN='true' ;;
    d) DEBUG="${OPTARG}" ;;
    \?) exit 1 ;;
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
echo "Building Handmade Hero"
mkdir -p ./build/bin/macos
pushd ./build/bin/macos
$CXX $CXX_FLAGS $OSX_LD_FLAGS -o $EXECUTABLE ../../../handmade/code/macos/macos_main.mm
popd

# Debug / Run
if [ -n "${DEBUG}" ]; then
    if [ "${DEBUG}" == "xcode" ]; then
        open -a Xcode ./debug/macos_debug/macos_debug.xcodeproj
        exit 1
    elif [ "${DEBUG}" == "lldb" ]; then
        lldb ./build/bin/macos/${EXECUTABLE}
        exit 1
    fi
elif [ "${RUN}" ]; then
    echo "Running Handmade Hero"
    ./build/bin/macos/${EXECUTABLE}
fi
