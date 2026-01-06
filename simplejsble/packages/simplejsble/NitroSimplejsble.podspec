require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroSimplejsble"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported, :visionos => 1.0 }

  s.source = { :path => "." }

  # Build SimpleBLE from source only if XCFramework doesn't exist
  # When installed from npm, the pre-built XCFramework is already included
  s.prepare_command = <<-CMD
    if [ -d "ios/SimpleBLE.xcframework" ]; then
      echo "SimpleBLE.xcframework already exists, skipping build"
    else
      echo "Building SimpleBLE for iOS..."
      chmod +x ios/build_simpleble.sh
      ios/build_simpleble.sh
    fi
  CMD

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

  # Required frameworks for SimpleBLE iOS backend
  s.frameworks = ['Foundation', 'CoreBluetooth']

  # Vendored XCFramework containing all architectures
  s.ios.vendored_frameworks = 'ios/SimpleBLE.xcframework'

  # Configure header search paths for SimpleBLE
  current_pod_target_xcconfig = s.attributes_hash['pod_target_xcconfig'] || {}
  s.pod_target_xcconfig = current_pod_target_xcconfig.merge({
    'HEADER_SEARCH_PATHS' => [
      '"$(inherited)"',
      '"$(PODS_TARGET_SRCROOT)/ios/simpleble_iphoneos_arm64/include"',
      '"$(PODS_TARGET_SRCROOT)/ios/simpleble_iphonesimulator_arm64/include"',
    ].join(' '),
  })
end
