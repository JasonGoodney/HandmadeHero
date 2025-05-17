#!/bin/bash

CXX="clang"

CXX_FLAGS="-g -Wall"

OSX_LD_FLAGS="-framework AppKit 
              -framework IOKit"

EXECUTABLE="handmade"
BUNDLE="Handmade"

# Flags
BUILD=''
CLEAN=''
DEBUG=''
RUN=''

while getopts 'bcd:r' flag; do
    case "${flag}" in
    b) BUILD='true' ;;
    c) CLEAN='true' ;;
    d) DEBUG="${OPTARG}" ;;
    r) RUN='true' ;;
    esac
done

# Clean
if [ "${CLEAN}" == true ]; then
    echo "Removing /build"
    rm -rf ./build
    if [ ! "${BUILD}" ] && [ ! "${RUN}" ] && [ ! "${DEBUG}" ]; then
        exit 1
    fi
fi

# Build
if [ "${BUILD}" ]; then
    echo "Building Handmade Hero"
    mkdir -p ./build/bin/macos
    pushd ./build/bin/macos
    $CXX $CXX_FLAGS $OSX_LD_FLAGS -o $EXECUTABLE ../../../handmade/code/macos/macos_main.mm
    popd

    # Bundle
    echo "Creating ${BUNDLE}.app Bundle"
    pushd ./build/bin/macos
    rm -rf "${BUNDLE}.app/"

    if [ ! -d "${BUNDLE}.app/Contents/MacOS" ]; then
        mkdir -p "${BUNDLE}.app/Contents/MacOS"
    fi

    if [ ! -f "${BUNDLE}.app/Contents/Info.plist" ]; then
        cat >"${BUNDLE}.app/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${BUNDLE}</string>
    <key>CFBundleIconFile</key>
    <string></string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${BUNDLE}</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF
    fi
    cp ${EXECUTABLE} ./${BUNDLE}.app/Contents/MacOS
    popd
fi

# Debug
if [ -n "${DEBUG}" ]; then
    if [ "${DEBUG}" == "xcode" ]; then
        open -a Xcode ./debug/macos_debug/macos_debug.xcodeproj
        exit 1
    elif [ "${DEBUG}" == "lldb" ]; then
        lldb ./build/bin/macos/${EXECUTABLE}
        exit 1
    fi
fi

# Run
if [ "${RUN}" ]; then
    echo "Running Handmade Hero"
    ./build/bin/macos/${EXECUTABLE}
fi
