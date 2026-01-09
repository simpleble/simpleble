#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MACOS_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET:-13.0}"

build_for_arch() {
    local ARCH=$1  # arm64 or x86_64

    local BUILD_DIR="${SCRIPT_DIR}/build_macosx_${ARCH}"
    local INSTALL_DIR="${SCRIPT_DIR}/simpleble_macosx_${ARCH}"

    echo "Building SimpleBLE for macOS (${ARCH})..."

    cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DCMAKE_OSX_SYSROOT=macosx \
        -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DSIMPLEBLE_EXCLUDE_C=ON

    cmake --build "${BUILD_DIR}" --config Release --parallel
    cmake --install "${BUILD_DIR}" --config Release

    echo "Installed to ${INSTALL_DIR}"
}

# Build for Apple Silicon
build_for_arch "arm64"

# Build for Intel
build_for_arch "x86_64"

# Create XCFramework from the static libraries
echo "Creating XCFramework..."
XCFRAMEWORK_PATH="${SCRIPT_DIR}/SimpleBLE-macos.xcframework"
rm -rf "${XCFRAMEWORK_PATH}"

# Create a universal (fat) library first, then wrap in XCFramework
# This is needed because XCFramework expects one library per platform
UNIVERSAL_DIR="${SCRIPT_DIR}/simpleble_macosx_universal"
mkdir -p "${UNIVERSAL_DIR}/lib"

lipo -create \
    "${SCRIPT_DIR}/simpleble_macosx_arm64/lib/libsimpleble.a" \
    "${SCRIPT_DIR}/simpleble_macosx_x86_64/lib/libsimpleble.a" \
    -output "${UNIVERSAL_DIR}/lib/libsimpleble.a"

# Copy headers from arm64 build (headers are the same for both architectures)
cp -R "${SCRIPT_DIR}/simpleble_macosx_arm64/include" "${UNIVERSAL_DIR}/"

xcodebuild -create-xcframework \
    -library "${UNIVERSAL_DIR}/lib/libsimpleble.a" \
    -headers "${UNIVERSAL_DIR}/include" \
    -output "${XCFRAMEWORK_PATH}"

echo "SimpleBLE macOS build complete!"
echo "XCFramework created at: ${XCFRAMEWORK_PATH}"
