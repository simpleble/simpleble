require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

# NOTE: scripts/build-ios.sh builds SimpleBLE into prebuilt/. SIMPLERNBLE_PLAIN=1
# at `pod install` links the PLAIN backend, for tests on the simulator.
flavor = ENV["SIMPLERNBLE_PLAIN"] == "1" ? "plain" : "release"

Pod::Spec.new do |s|
  s.name         = "SimpleRnBLE"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = "The California Open Source Company"
  s.platforms    = { :ios => "15.8" }
  s.source       = { :git => "https://github.com/simpleble/simpleble.git", :tag => "v#{s.version}" }

  s.source_files = "cpp/**/*.{hpp,cpp}"
  s.vendored_frameworks = "prebuilt/#{flavor}/SimpleBLE.xcframework"
  s.preserve_paths = "prebuilt/#{flavor}/include/**/*"
  s.frameworks = "CoreBluetooth"
  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/prebuilt/#{flavor}/include\"",
    "GCC_PREPROCESSOR_DEFINITIONS" => "$(inherited) " + (flavor == "plain" ? "SIMPLEBLE_BACKEND_PLAIN=1" : "SIMPLEBLE_BACKEND_IOS=1"),
  }

  load "nitrogen/generated/ios/SimpleRnBLE+autolinking.rb"
  add_nitrogen_files(s)

  install_modules_dependencies(s)
end
