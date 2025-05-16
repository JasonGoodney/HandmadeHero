#!/bin/bash

echo "Building Handmade Hero"

CXX="clang"

CXX_FLAGS="-g -Wall"

OSX_LD_FLAGS="-framework AppKit 
              -framework IOKit"

OUTPUT="handmade"

SRC_DIR="../../handmade/code/"

mkdir -p ./build/bin
pushd ./build/bin
$CXX $CXX_FLAGS $OSX_LD_FLAGS -o $OUTPUT $SRC_DIR/macos/macos_main.mm
popd