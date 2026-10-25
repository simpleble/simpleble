// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "SimpleSwiftBLEExamples",
    platforms: [.macOS(.v15)],
    dependencies: [
        .package(name: "SimpleSwiftBLE", path: "../..")
    ],
    targets: [
        .executableTarget(name: "list_adapters", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "list_backends", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "power_cycle", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "scan", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "connect", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "read", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "write", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "notify", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "peripheral", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
        .executableTarget(name: "dongl", dependencies: [.product(name: "SimpleSwiftBLE", package: "SimpleSwiftBLE")]),
    ]
)
