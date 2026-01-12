require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroSimplejsble"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = {
    :ios => '13.4',
    :osx => '10.13'
  }

  s.source = { :path => "." }

  # XCFramework is pre-built during npm publish (scripts/build-xcframework.sh)
  # No prepare_command needed - the XCFramework is included in the npm package

  s.source_files = [
    # Implementation (Swift)
    "ios/**/*.{swift}",
    # Autolinking/Registration (Objective-C++)
    "ios/**/*.{m,mm}",
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

  # Required frameworks for SimpleBLE
  s.frameworks = ['Foundation', 'CoreBluetooth']
  # macOS requires additional frameworks for classic Bluetooth support
  s.osx.frameworks = ['Foundation', 'CoreBluetooth', 'IOBluetooth', 'IOKit']

  # Vendored XCFramework containing all architectures (iOS + macOS)
  s.ios.vendored_frameworks = 'ios/SimpleBLE.xcframework'
  s.osx.vendored_frameworks = 'ios/SimpleBLE.xcframework'

  # Configure header search paths for SimpleBLE XCFramework
  # Headers are inside the XCFramework slices - we include all platforms so it works for any target
  current_pod_target_xcconfig = s.attributes_hash['pod_target_xcconfig'] || {}
  s.pod_target_xcconfig = current_pod_target_xcconfig.merge({
    'HEADER_SEARCH_PATHS' => [
      '"$(inherited)"',
      # XCFramework headers for each platform slice
      '"$(PODS_TARGET_SRCROOT)/ios/SimpleBLE.xcframework/ios-arm64/Headers"',
      '"$(PODS_TARGET_SRCROOT)/ios/SimpleBLE.xcframework/ios-arm64-simulator/Headers"',
      '"$(PODS_TARGET_SRCROOT)/ios/SimpleBLE.xcframework/macos-arm64_x86_64/Headers"',
    ].join(' '),
  })
end
