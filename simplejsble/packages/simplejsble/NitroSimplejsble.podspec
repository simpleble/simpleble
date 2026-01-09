require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroSimplejsble"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported, :visionos => 1.0, :osx => '13.0' }

  s.source = { :path => "." }

  # Build SimpleBLE from source only if XCFramework doesn't exist
  # When installed from npm, the pre-built XCFramework is already included
  s.prepare_command = <<-CMD
    # Build iOS XCFramework if not present
    if [ -d "ios/SimpleBLE.xcframework" ]; then
      echo "SimpleBLE.xcframework (iOS) already exists, skipping build"
    else
      echo "Building SimpleBLE for iOS..."
      chmod +x ios/build_simpleble.sh
      ios/build_simpleble.sh
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
  s.exclude_files = ["ios/CMakeLists.txt", "macos/CMakeLists.txt"]

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
