#!/bin/bash
set -e

# Get script directory and calculate paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${PACKAGE_DIR}/.." && pwd)"
APPLE_DIR="${PACKAGE_DIR}/apple"

# Deployment targets
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-15.8}"
MACOS_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET:-13}"

# Directories to clean before packing (native build artifacts only, not lib/)
DIRS_TO_CLEAN=(
  "android/build"
  "android/.cxx"
  "android/.gradle"
  "android/prebuilt"
  "apple/build"
)

# Directories/files to copy from repo root
ITEMS_TO_COPY=(
  "simpledroidbridge"
  "VERSION"
  "LICENSE.md"
)

echo "Preparing package for npm publish..."
echo "Package dir: ${PACKAGE_DIR}"
echo "Repo root: ${REPO_ROOT}"

# Clean build artifacts first
echo ""
echo "Cleaning build artifacts..."
for dir in "${DIRS_TO_CLEAN[@]}"; do
  dir_path="${PACKAGE_DIR}/${dir}"
  if [ -d "${dir_path}" ]; then
    rm -rf "${dir_path}"
    echo "✓ Cleaned: ${dir}"
  fi
done
echo ""

# Copy items from repo root
for item in "${ITEMS_TO_COPY[@]}"; do
  src="${REPO_ROOT}/${item}"
  dest="${PACKAGE_DIR}/${item}"

  if [ ! -e "${src}" ]; then
    echo "Warning: ${src} does not exist, skipping..."
    continue
  fi

  # Remove existing copy if it exists
  if [ -e "${dest}" ]; then
    rm -rf "${dest}"
  fi

  # Copy directory or file
  if [ -d "${src}" ]; then
    cp -R "${src}" "${dest}"
    echo "✓ Copied directory: ${item}"
  else
    cp "${src}" "${dest}"
    echo "✓ Copied file: ${item}"
  fi
done

# =============================================================================
# Build SimpleBLE XCFramework for iOS and macOS
# =============================================================================

echo ""
echo "Building SimpleBLE XCFramework..."
echo "iOS deployment target: ${IOS_DEPLOYMENT_TARGET}"
echo "macOS deployment target: ${MACOS_DEPLOYMENT_TARGET}"

# Build for iOS platforms (iphoneos, iphonesimulator)
build_for_ios_platform() {
    local PLATFORM=$1  # iphoneos or iphonesimulator
    local ARCH=$2      # arm64 or x86_64

    local BUILD_DIR="${APPLE_DIR}/build_${PLATFORM}_${ARCH}"
    local INSTALL_DIR="${APPLE_DIR}/simpleble_${PLATFORM}_${ARCH}"

    echo ""
    echo "Building SimpleBLE for ${PLATFORM} (${ARCH})..."

    cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}/simpleble" \
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

# Build for macOS platforms
build_for_macos_platform() {
    local ARCH=$1  # arm64 or x86_64

    local BUILD_DIR="${APPLE_DIR}/build_macosx_${ARCH}"
    local INSTALL_DIR="${APPLE_DIR}/simpleble_macosx_${ARCH}"

    echo ""
    echo "Building SimpleBLE for macOS (${ARCH})..."

    cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}/simpleble" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DSIMPLEBLE_EXCLUDE_C=ON

    cmake --build "${BUILD_DIR}" --config Release --parallel
    cmake --install "${BUILD_DIR}" --config Release

    echo "Installed to ${INSTALL_DIR}"
}

# Build for iOS device
build_for_ios_platform "iphoneos" "arm64"

# Build for iOS simulator (Apple Silicon)
build_for_ios_platform "iphonesimulator" "arm64"

# Build for macOS (Apple Silicon)
build_for_macos_platform "arm64"

# Build for macOS (Intel)
build_for_macos_platform "x86_64"

# Create universal macOS library
echo ""
echo "Creating universal macOS library..."
MACOS_UNIVERSAL_DIR="${APPLE_DIR}/simpleble_macosx_universal"
mkdir -p "${MACOS_UNIVERSAL_DIR}/lib"
cp -r "${APPLE_DIR}/simpleble_macosx_arm64/include" "${MACOS_UNIVERSAL_DIR}/"
lipo -create \
    "${APPLE_DIR}/simpleble_macosx_arm64/lib/libsimpleble.a" \
    "${APPLE_DIR}/simpleble_macosx_x86_64/lib/libsimpleble.a" \
    -output "${MACOS_UNIVERSAL_DIR}/lib/libsimpleble.a"

# Create XCFramework from the static libraries (iOS + macOS)
echo ""
echo "Creating XCFramework..."
XCFRAMEWORK_PATH="${APPLE_DIR}/SimpleBLE.xcframework"
rm -rf "${XCFRAMEWORK_PATH}"

xcodebuild -create-xcframework \
    -library "${APPLE_DIR}/simpleble_iphoneos_arm64/lib/libsimpleble.a" \
    -headers "${APPLE_DIR}/simpleble_iphoneos_arm64/include" \
    -library "${APPLE_DIR}/simpleble_iphonesimulator_arm64/lib/libsimpleble.a" \
    -headers "${APPLE_DIR}/simpleble_iphonesimulator_arm64/include" \
    -library "${MACOS_UNIVERSAL_DIR}/lib/libsimpleble.a" \
    -headers "${MACOS_UNIVERSAL_DIR}/include" \
    -output "${XCFRAMEWORK_PATH}"

echo ""
echo "SimpleBLE build complete!"
echo "XCFramework created at: ${XCFRAMEWORK_PATH}"

# Clean up intermediate build directories to reduce package size
echo ""
echo "Cleaning up intermediate build directories..."
rm -rf "${APPLE_DIR}/build_iphoneos_arm64"
rm -rf "${APPLE_DIR}/build_iphonesimulator_arm64"
rm -rf "${APPLE_DIR}/build_macosx_arm64"
rm -rf "${APPLE_DIR}/build_macosx_x86_64"
rm -rf "${APPLE_DIR}/simpleble_iphoneos_arm64"
rm -rf "${APPLE_DIR}/simpleble_iphonesimulator_arm64"
rm -rf "${APPLE_DIR}/simpleble_macosx_arm64"
rm -rf "${APPLE_DIR}/simpleble_macosx_x86_64"
rm -rf "${APPLE_DIR}/simpleble_macosx_universal"

# =============================================================================
# Build SimpleBLE static libraries for Android
# =============================================================================

echo ""
echo "Building SimpleBLE for Android..."

ANDROID_DIR="${PACKAGE_DIR}/android"
ANDROID_PREBUILT_DIR="${ANDROID_DIR}/prebuilt"

# Detect NDK path from environment variables (CI-compatible)
# Priority: ANDROID_NDK_HOME > ANDROID_NDK > ANDROID_HOME/ndk > ANDROID_SDK_ROOT/ndk
detect_ndk_from_sdk() {
    local sdk_root="${1}"
    if [ -n "$sdk_root" ] && [ -d "$sdk_root/ndk" ]; then
        # Find the latest NDK version in the SDK
        local ndk_path=$(ls -d "$sdk_root/ndk"/*/ 2>/dev/null | sort -V | tail -1)
        if [ -n "$ndk_path" ]; then
            echo "${ndk_path%/}"  # Remove trailing slash
            return 0
        fi
    fi
    return 1
}

NDK_PATH=""
if [ -n "${ANDROID_NDK_HOME}" ]; then
    NDK_PATH="${ANDROID_NDK_HOME}"
elif [ -n "${ANDROID_NDK}" ]; then
    NDK_PATH="${ANDROID_NDK}"
elif NDK_PATH=$(detect_ndk_from_sdk "${ANDROID_HOME}"); then
    : # NDK_PATH already set by detect_ndk_from_sdk
elif NDK_PATH=$(detect_ndk_from_sdk "${ANDROID_SDK_ROOT}"); then
    : # NDK_PATH already set by detect_ndk_from_sdk
fi

if [ -z "${NDK_PATH}" ] || [ ! -d "${NDK_PATH}" ]; then
    echo "Warning: Android NDK not found. Skipping Android prebuilt libraries."
    echo "To enable Android builds, set one of these environment variables:"
    echo "  - ANDROID_NDK_HOME (direct path to NDK)"
    echo "  - ANDROID_HOME (SDK path, NDK will be found in \$ANDROID_HOME/ndk/)"
    echo "  - ANDROID_SDK_ROOT (same as ANDROID_HOME)"
else
    echo "Using NDK: ${NDK_PATH}"

    # Android ABIs to build
    ANDROID_ABIS=("arm64-v8a" "armeabi-v7a" "x86_64" "x86")
    ANDROID_API_LEVEL="${ANDROID_API_LEVEL:-24}"

    # Clean previous prebuilt directory
    rm -rf "${ANDROID_PREBUILT_DIR}"
    mkdir -p "${ANDROID_PREBUILT_DIR}"

    # Build for each ABI
    build_for_android_abi() {
        local ABI=$1
        local BUILD_DIR="${ANDROID_DIR}/build_android_${ABI}"
        local INSTALL_DIR="${ANDROID_PREBUILT_DIR}/${ABI}"

        echo ""
        echo "Building SimpleBLE for Android ${ABI}..."

        mkdir -p "${BUILD_DIR}"
        mkdir -p "${INSTALL_DIR}"

        cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}/simpleble" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE="${NDK_PATH}/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI="${ABI}" \
            -DANDROID_PLATFORM="android-${ANDROID_API_LEVEL}" \
            -DANDROID_STL=c++_shared \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
            -DBUILD_SHARED_LIBS=OFF \
            -DSIMPLEBLE_EXCLUDE_C=ON

        cmake --build "${BUILD_DIR}" --config Release --parallel
        cmake --install "${BUILD_DIR}" --config Release

        echo "Installed to ${INSTALL_DIR}"
    }

    for ABI in "${ANDROID_ABIS[@]}"; do
        build_for_android_abi "${ABI}"
    done

    # Clean up intermediate build directories
    echo ""
    echo "Cleaning up Android intermediate build directories..."
    for ABI in "${ANDROID_ABIS[@]}"; do
        rm -rf "${ANDROID_DIR}/build_android_${ABI}"
    done

    echo ""
    echo "Android SimpleBLE build complete!"
    echo "Prebuilt libraries at: ${ANDROID_PREBUILT_DIR}"
fi

echo ""
echo "Package preparation complete!"
