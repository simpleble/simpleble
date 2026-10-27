# SimpleSwiftBLE development

This is contributor documentation for SimpleSwiftBLE, the Swift bindings. It is kept outside `content/docs` and is
not included in the published documentation site.

## Layout

```text
Package.swift                     The package for development, using the xcframework the build script builds
simpleswiftble/
├── Sources/SimpleSwiftBLE/       Public API, over the SimpleCBLE C API
├── Tests/SimpleSwiftBLETests/    Tests against PLAIN
├── scripts/build-xcframework.sh  Builds SimpleCBLE into build/<flavor>/CSimpleBLE.xcframework
├── scripts/package.sh            Assembles the package that simpleble/simpleswiftble publishes
├── .spi.yml                      Swift Package Index configuration, published with the package
└── build/                        Generated, ignored by git
```

Applications depend on [simpleble/simpleswiftble](https://github.com/simpleble/simpleswiftble), which CI generates.
Its `.spi.yml` lets the [Swift Package Index](https://swiftpackageindex.com) build and host the DocC documentation of
`SimpleSwiftBLE` once the package is listed there.
`Package.swift` stays at the repository root because SwiftPM names a local package after its folder: in
`simpleswiftble/`, it would clash with `examples/simpleswiftble`, whose package depends on it.

## Build and test

```bash
PLAIN=1 simpleswiftble/scripts/build-xcframework.sh
SIMPLESWIFTBLE_FLAVOR=plain swift test
```

`Package.swift` uses the xcframework the script built: the native build, which the script builds without `PLAIN=1`,
unless `SIMPLESWIFTBLE_FLAVOR=plain` selects the PLAIN build. The variable also reaches `xcodebuild`, which runs the
tests on the iOS simulator and as Mac Catalyst:

```bash
SIMPLESWIFTBLE_FLAVOR=plain xcodebuild test -scheme SimpleSwiftBLE -destination 'platform=iOS Simulator,name=iPhone 17'
SIMPLESWIFTBLE_FLAVOR=plain xcodebuild test -scheme SimpleSwiftBLE -destination 'platform=macOS,variant=Mac Catalyst'
```

## CI distribution

`ci_simpleswiftble.yml`, called by `ci_main.yml`, builds both xcframework flavors on macOS, runs the PLAIN tests on
macOS and on an iPhone simulator, builds the examples against the native build, and checks the manifest
`scripts/package.sh` generates.

## Versions and publication

SwiftPM resolves versions from Git tags, so SimpleSwiftBLE is published like SimpleGoBLE. For each `main` build and
each published SimpleBLE release, `publish-simpleswiftble` in `ci_main.yml` updates the generated
[simpleble/simpleswiftble](https://github.com/simpleble/simpleswiftble) repository using `SIMPLESWIFTBLE_TOKEN`, a token
with read and write access to its contents:

1. It builds the native xcframework and attaches `CSimpleBLE.xcframework.zip` to a draft release `vX.Y.Z` or
   `vX.Y.Z-dev.N` there. Development builds are prereleases.
2. It replaces the repository's files with the output of `scripts/package.sh`: the sources, README, license, and a
   `Package.swift` that points at that archive and its checksum.
3. It pushes the commit to `main` and its tag in one atomic push, then publishes the release.

A retried run reuses the archive in the draft an earlier attempt made, so the checksum in `Package.swift` stays
valid. If the tag already exists, the run only publishes its release if it is still a draft. Never replace a published
tag or its archive.

Development builds are never deleted. Each has its own release with one archive, so GitHub's limit of 1000 assets per
release never applies. Applications follow the latest development build with `branch: "main"`. SwiftPM compares the number after `-dev.`
as a number, so `1.3.0-dev.99` sorts before `1.3.0-dev.100`, and a single development build is pinned with `exact:`.

## How a call reaches SimpleBLE

The Swift code calls SimpleCBLE through the `CSimpleBLE` module, imported with `internal import`, so applications
see only Swift types. Swift's C++ interoperability is not used: a C++ exception that reaches Swift ends the program,
and SimpleBLE reports every failure as one.

- **Identity.** SimpleCBLE returns a new handle every time. The bindings keep one `Adapter` per
  `simpleble_adapter_identity()`, one `Peripheral` per `simpleble_peripheral_identity()`, and one `LocalService` and
  `LocalCharacteristic` per identity, and one `Backend` per identifier. They keep the first handle and release later
  ones. None of these objects is ever released, as SimpleBLE keeps its own, and local peripherals are kept too, as in
  SimpleKtBLE: hosting ends with `stop()`.
- **Callbacks.** Each native object's callbacks are registered once, when its Swift object is created. Their
  userdata is a number the callback looks up: the identity, or a counter for local peripherals and subscriptions. It
  is never a pointer to a Swift object, since SimpleCBLE has no notification that a callback was released. Callbacks
  are registered outside the bindings' locks, because SimpleBLE may hold its own locks while it calls them.
- **Events.** Each event is an `AsyncStream` that buffers without limit. A stream is registered when it is created,
  so events that happen before its iteration starts are not lost.
- **Subscriptions.** `notify()` and `indicate()` claim the characteristic, subscribe, and return the stream. The
  disconnection callback ends the subscriptions the peripheral had, after the values that came before it. Only a
  stream that is cancelled or dropped while it still holds its claim unsubscribes natively.
- **Blocking calls.** Calls that wait on the Bluetooth stack run on a private concurrent `DispatchQueue` and resume
  their caller through a continuation, so they never block Swift's cooperative thread pool.
- **Handlers.** The read handler runs on the SimpleBLE thread that waits for it, and its value is copied into a buffer
  kept until the next read, as SimpleCBLE requires. The Dongl pairing handlers are `async`: the SimpleBLE thread that
  waits for the answer blocks on a semaphore while a task runs the handler.
- **Properties.** Properties return state the library already holds and don't throw. Anything that waits on the
  stack or can fail is a function, as in SimpleKtBLE.

## Tests

The tests check only the binding layer against PLAIN's fixture: callbacks reaching streams, object identity across
streams and lists, conversions, and error codes. Every suite is nested in one serialized suite, since PLAIN's state
lasts for the whole process.
