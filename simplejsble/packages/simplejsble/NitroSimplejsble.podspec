require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroSimplejsble"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported, :osx => '13.0' }

  s.source = { :path => "." }

  # Build SimpleBLE from source only if XCFramework doesn't exist
  # When installed from npm, the pre-built XCFramework is already included
  s.prepare_command = <<-CMD
    # Build iOS XCFramework if not present
    if [ -d "ios/SimpleBLE.xcframework" ]; then
      echo "SimpleBLE.xcframework (iOS) already exists, skipping build"
    else
      echo "Building SimpleBLE for iOS..."
      set -e
      IOS_DIR="ios"
      IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-13.4}"
      
      build_for_platform() {
          local PLATFORM=$1  # iphoneos or iphonesimulator
          local ARCH=$2      # arm64 or x86_64
      
          local BUILD_DIR="${IOS_DIR}/build_${PLATFORM}_${ARCH}"
          local INSTALL_DIR="${IOS_DIR}/simpleble_${PLATFORM}_${ARCH}"
      
          echo "Building SimpleBLE for ${PLATFORM} (${ARCH})..."
      
          cmake -B "${BUILD_DIR}" -S "${IOS_DIR}" \
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
      XCFRAMEWORK_PATH="${IOS_DIR}/SimpleBLE.xcframework"
      rm -rf "${XCFRAMEWORK_PATH}"
      
      xcodebuild -create-xcframework \
          -library "${IOS_DIR}/simpleble_iphoneos_arm64/lib/libsimpleble.a" \
          -headers "${IOS_DIR}/simpleble_iphoneos_arm64/include" \
          -library "${IOS_DIR}/simpleble_iphonesimulator_arm64/lib/libsimpleble.a" \
          -headers "${IOS_DIR}/simpleble_iphonesimulator_arm64/include" \
          -output "${XCFRAMEWORK_PATH}"
      
      echo "SimpleBLE build complete!"
      echo "XCFramework created at: ${XCFRAMEWORK_PATH}"
    fi

    # Build macOS XCFramework if not present
    if [ -d "macos/SimpleBLE-macos.xcframework" ]; then
      echo "SimpleBLE-macos.xcframework already exists, skipping build"
    else
      echo "Building SimpleBLE for macOS..."
      chmod +x macos/build_simpleble.sh
      macos/build_simpleble.sh
    fi
  CMD

  s.source_files = [
    # Implementation (Swift)
    "ios/**/*.{swift}",
    "macos/**/*.{swift}",
    # Autolinking/Registration (Objective-C++)
    "ios/**/*.{m,mm}",
    "macos/**/*.{m,mm}",
    # Implementation (C++ objects)
    "cpp/**/*.{hpp,cpp}",
  ]

  # Exclude CMakeLists.txt from source files
  s.exclude_files = ["ios/CMakeLists.txt"]

  load 'nitrogen/generated/ios/NitroSimplejsble+autolinking.rb'
  add_nitrogen_files(s)

  s.dependency 'React-jsi'
  s.dependency 'React-callinvoker'
  install_modules_dependencies(s)

  # Required frameworks for SimpleBLE (shared by iOS and macOS)
  s.frameworks = ['Foundation', 'CoreBluetooth']

  # macOS-specific frameworks (IOBluetooth/IOKit for USB dongle support)
  s.osx.frameworks = ['Foundation', 'CoreBluetooth', 'IOBluetooth', 'IOKit']

  # Vendored XCFramework containing all architectures
  s.ios.vendored_frameworks = 'ios/SimpleBLE.xcframework'
  s.osx.vendored_frameworks = 'macos/SimpleBLE-macos.xcframework'

  # Configure header search paths for SimpleBLE
  current_pod_target_xcconfig = s.attributes_hash['pod_target_xcconfig'] || {}
  s.pod_target_xcconfig = current_pod_target_xcconfig.merge({
    'HEADER_SEARCH_PATHS' => [
      '"$(inherited)"',
      # iOS header paths
      '"$(PODS_TARGET_SRCROOT)/ios/simpleble_iphoneos_arm64/include"',
      '"$(PODS_TARGET_SRCROOT)/ios/simpleble_iphonesimulator_arm64/include"',
      # macOS header paths
      '"$(PODS_TARGET_SRCROOT)/macos/simpleble_macosx_arm64/include"',
      '"$(PODS_TARGET_SRCROOT)/macos/simpleble_macosx_x86_64/include"',
    ].join(' '),
  })
end
