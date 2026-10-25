// swift-tools-version: 6.0

import PackageDescription

// NOTE: In this repository the package uses the xcframework that simpleswiftble/scripts/build-xcframework.sh builds.
// SIMPLESWIFTBLE_FLAVOR=plain selects the PLAIN build the tests use.
let flavor = Context.environment["SIMPLESWIFTBLE_FLAVOR"] ?? "release"

let package = Package(
    name: "SimpleSwiftBLE",
    platforms: [.macOS(.v15), .iOS("15.8"), .macCatalyst(.v16)],
    products: [
        .library(name: "SimpleSwiftBLE", targets: ["SimpleSwiftBLE"])
    ],
    targets: [
        .binaryTarget(name: "CSimpleBLE", path: "simpleswiftble/build/\(flavor)/CSimpleBLE.xcframework"),
        .target(
            name: "SimpleSwiftBLE",
            dependencies: ["CSimpleBLE"],
            path: "simpleswiftble/Sources/SimpleSwiftBLE",
            linkerSettings: [
                .linkedFramework("CoreBluetooth"),
                .linkedFramework("IOBluetooth", .when(platforms: [.macOS])),
                .linkedFramework("IOKit", .when(platforms: [.macOS])),
            ]
        ),
        .testTarget(
            name: "SimpleSwiftBLETests",
            dependencies: ["SimpleSwiftBLE"],
            path: "simpleswiftble/Tests/SimpleSwiftBLETests"
        ),
    ]
)
