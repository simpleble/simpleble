#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-13.4}"

build_for_platform() {
    local PLATFORM=$1  # iphoneos or iphonesimulator
    local ARCH=$2      # arm64 or x86_64

    local BUILD_DIR="${SCRIPT_DIR}/build_${PLATFORM}_${ARCH}"
    local INSTALL_DIR="${SCRIPT_DIR}/simpleble_${PLATFORM}_${ARCH}"

    echo "Building SimpleBLE for ${PLATFORM} (${ARCH})..."

    cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT="${PLATFORM}" \
        -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DSIMPLEBLE_EXCLUDE_C=ON

    cmake --build "${BUILD_DIR}" --config Release --parallel
    cmake --install "${BUILD_DIR}" --config Release

    echo "Installed to ${INSTALL_DIR}"
}

# Build for device
build_for_platform "iphoneos" "arm64"

# Build for simulator (Apple Silicon)
build_for_platform "iphonesimulator" "arm64"

# Create XCFramework from the static libraries
echo "Creating XCFramework..."
XCFRAMEWORK_PATH="${SCRIPT_DIR}/SimpleBLE.xcframework"
rm -rf "${XCFRAMEWORK_PATH}"

xcodebuild -create-xcframework \
    -library "${SCRIPT_DIR}/simpleble_iphoneos_arm64/lib/libsimpleble.a" \
    -headers "${SCRIPT_DIR}/simpleble_iphoneos_arm64/include" \
    -library "${SCRIPT_DIR}/simpleble_iphonesimulator_arm64/lib/libsimpleble.a" \
    -headers "${SCRIPT_DIR}/simpleble_iphonesimulator_arm64/include" \
    -output "${XCFRAMEWORK_PATH}"

echo "SimpleBLE build complete!"
echo "XCFramework created at: ${XCFRAMEWORK_PATH}"
