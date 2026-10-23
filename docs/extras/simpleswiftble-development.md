# SimpleSwiftBLE development

This is contributor documentation for SimpleSwiftBLE, the Swift bindings. It is kept outside `content/docs` and is
not included in the published documentation site.

## Layout

```text
Package.swift                     The Swift package, at the repository root as Swift Package Manager requires
simpleswiftble/
├── Sources/SimpleSwiftBLE/       Public API, over the SimpleCBLE C API
├── Tests/SimpleSwiftBLETests/    Tests against PLAIN
├── scripts/build-xcframework.sh  Builds SimpleCBLE into build/<flavor>/CSimpleBLE.xcframework
├── scripts/release-manifest.sh   Points Package.swift at a release's xcframework archive, for publishing
└── build/                        Generated, ignored by git
```

## Build and test

```bash
PLAIN=1 simpleswiftble/scripts/build-xcframework.sh
SIMPLESWIFTBLE_FLAVOR=plain swift test
```

In this repository, `Package.swift` uses the xcframework the script built: the native build, which the script builds
without `PLAIN=1`, unless `SIMPLESWIFTBLE_FLAVOR=plain` selects the PLAIN build. The variable also reaches
`xcodebuild`, which runs the tests on the iOS simulator and as Mac Catalyst:

```bash
SIMPLESWIFTBLE_FLAVOR=plain xcodebuild test -scheme SimpleSwiftBLE -destination 'platform=iOS Simulator,name=iPhone 17'
SIMPLESWIFTBLE_FLAVOR=plain xcodebuild test -scheme SimpleSwiftBLE -destination 'platform=macOS,variant=Mac Catalyst'
```

## CI and publishing

`ci_simpleswiftble.yml`, called by `ci_main.yml`, builds both xcframework flavors on macOS, runs the PLAIN tests on
macOS and on an iPhone simulator, and builds the examples against the native build.

SwiftPM resolves versions from tags. For a published GitHub release, `publish-simpleswiftble` in `ci_main.yml`:

1. Builds the native xcframework and uploads it to the release as `simpleswiftble-xcframework.zip`.
2. Runs `scripts/release-manifest.sh`, which points `Package.swift` at that archive and its checksum.
3. Commits that change on top of the release commit `vX.Y.Z` and pushes only the tag `X.Y.Z`.

SwiftPM prefers `X.Y.Z` over `vX.Y.Z`. The commit isn't on `main`, so the development version numbering, which counts
commits since the latest tag on `main`, doesn't see it.

The archive isn't a workflow artifact, so `publish-release` never uploads or replaces it. A retried run reuses an
archive an earlier attempt uploaded, so the checksum a version pins stays valid. If the tag already exists, the job
does nothing when the tag points at this release's archive, and fails otherwise.

Until the `X.Y.Z` tag exists, SwiftPM resolves `vX.Y.Z`, whose `Package.swift` expects a local build, and resolution
fails. Main builds get no SwiftPM version; test development changes from a checkout.

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
