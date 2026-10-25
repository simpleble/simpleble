# SimpleDartBLE development

This is contributor documentation for SimpleDartBLE, the Dart bindings, and SimpleFlutterBLE, the Flutter bindings
built on them. It is kept outside
`content/docs` and is not included in the published documentation site.

## Layout

```text
simpledartble/
├── lib/
│   ├── simpledartble.dart        Public API exports
│   └── src/
│       ├── adapter.dart, ...     Public classes
│       └── internal/
│           ├── bindings.g.dart   dart:ffi bindings, generated from the SimpleCBLE headers
│           ├── advanced.dart     The platform functions ffigen leaves out
│           ├── native.dart       Strings, bytes, UUIDs and errors across the boundary
│           ├── events.dart       Delivers SimpleBLE callbacks to Dart objects
│           ├── event_stream.dart Broadcast streams of those callbacks
│           ├── workers.dart      Helper isolates for blocking calls, strands, keep-alive
│           ├── registry.dart     One Dart object per native object
│           └── native_library.dart  The CMake build of one target, for the hook and tool/prebuild.dart
├── hook/build.dart               Builds the native library, or takes the published one from prebuilt/
├── native/                       CMake project and the C++ shim beside SimpleCBLE
├── test/                         Binding tests, against PLAIN
└── tool/
    ├── ffigen.dart               Regenerates lib/src/internal/bindings.g.dart
    ├── test_app.dart             Runs the tests inside a Flutter app
    ├── prebuild.dart             Builds the published libraries
    └── prepublish.dart           Prepares both packages for publishing

simpleflutterble/
├── lib/simpleflutterble.dart     Re-exports SimpleDartBLE's API
└── android/                      Initializer, simpledroidbridge classes and permissions
```

`simpledartble` is a pure Dart package that works in Dart programs and Flutter apps alike. Flutter apps on Android also
need Java code and manifest entries, which only a Flutter plugin can add, and pub installs a Flutter plugin only with
the Flutter SDK, so `simpleflutterble` is the package Flutter apps use: it re-exports SimpleDartBLE's API and carries
the Android module, as realm does for realm_dart. The module and the native library call each other, so both packages
always have the same version, and `pubspec_overrides.yaml` resolves simpledartble from the checkout. Web has no
`dart:ffi`.

## How a call reaches SimpleBLE

Every binding call is the matching SimpleCBLE call through `@Native` functions in `bindings.g.dart`. Regenerate them
with `dart run tool/ffigen.dart` after changing a SimpleCBLE header or `native/include`.

The native library holds SimpleCBLE and SimpleBLE, linked statically, and a small shim (`native/src`). The shim only
does what Dart can't:

- **Callbacks.** Dart registers SimpleCBLE callbacks with the shim's `simpledartble_on_*` functions and a token as
  userdata. They copy what SimpleCBLE lends for the call and post `[token, value]` to the port of the isolate that
  claimed the events, with `Dart_PostCObject`. Posting to the port of an isolate that has shut down fails instead of
  crashing, which matters after a Flutter hot restart, when SimpleBLE keeps calling back. A peripheral handle that
  nothing receives is released. Tokens are unique in the process, so a stale registration never reaches a new object.
- **Questions.** Read handlers and the Dongl passkey request and numeric comparison callbacks wait for an answer.
  Their `simpledartble_on_*` functions post `[token, question, argument]` and block the SimpleBLE thread until Dart
  calls `simpledartble_answer_*`, like SimpleJSBLE's `ask()`. Without an answer, because the handler failed or the
  isolate is gone, they get a fallback: an empty value for a read, a rejection for pairing. Handlers are installed
  from workers, because the core holds a callback's lock while it runs, and they run in the zone they were set in.
- **One isolate at a time** claims the events, like SimpleJSBLE's one Node environment, as callbacks belong to the
  native objects every isolate shares. A claim succeeds once the previous owner's port is closed, so a hot-restarted
  isolate takes over, and test files run one at a time (`dart_test.yaml`).
- **Shutdown.** Posting to the Dart VM after it shut down crashes, and SimpleBLE calls back while a program ends, for
  example to report a disconnection. A `NativeFinalizer` on the event receiver calls `simpledartble_release` when the
  isolate group shuts down, which ends the claim under the lock every post takes, so nothing posts afterwards.
- **Android.** The Dart VM opens the library with dlopen, which doesn't run `JNI_OnLoad`. SimpleFlutterBLE's
  `SimpleDartBleInitializer`, a ContentProvider, loads it with `System.loadLibrary` as the app starts, so SimpleBLE gets
  the JVM, then passes the application context and starts the backend on that thread, where the bridge classes can be
  found. The module compiles the simpledroidbridge sources. Without it, `simpledartble_platform_ready()` stays false,
  and the calls that reach a backend throw a `StateError` that names the package.

Getters return state that is already known and call SimpleCBLE directly. Anything that waits on the Bluetooth stack
returns a future: the call joins its object's strand and runs on a helper isolate, so calls on one object run in
call order and a long connection attempt never holds up another object. Workers get handle addresses and top-level
functions, never Dart objects; `Strand.call` takes the function and its argument separately so its closure captures
nothing else. Backend enumeration and handle releases share the `lifecycle` strand, because releasing a Dongl adapter
closes its serial port.

There is one Dart object per native object, keyed by SimpleCBLE's identity functions, and backends by their
identifier. A duplicate handle is released at once. Objects with listeners or subscriptions, scanning adapters,
connected peripherals and started local peripherals stay alive; the others release their handles when collected. As in
SimpleJSBLE, a pending call, an active scan, an open connection or a started local peripheral keeps a Dart program
running, and listeners don't. Some backends report a scan or connection starting or ending only after the call
returns, so both the call and the event record it.

SimpleCBLE declares the iOS, macOS and Linux functions of `advanced.h` only for their own platform, so
`tool/ffigen.dart` leaves them out, and `lib/src/internal/advanced.dart` declares them by hand.

## Native builds

In a checkout, `hook/build.dart` runs CMake on `native/` for the target Dart or Flutter asks for, with the sources
beside the package. The published package has no sources beside it, so the hook takes the target's library from
`prebuilt/` instead; see [publishing](simpledartble-publishing.md).
The user-define `backend: plain` builds the PLAIN backend. Only the root package's user-defines apply, so the one in
`simpledartble/pubspec.yaml` covers this package's own tests and never an app that depends on it. Hooks see only a
few environment variables, so there is no environment switch.

- macOS and iOS build for 15.0 and 15.8. Flutter passes hooks fixed Apple versions, so the hook sets its own. Flutter
  wraps the library in a framework.
- Android builds for API 31 or the app's `minSdk`, with the static C++ runtime.
- Windows builds with Visual Studio and the static CRT. A static SimpleCBLE's export macro is empty on MSVC, so
  `native/CMakeLists.txt` predefines it.
- Linux needs the D-Bus development package.

Building needs CMake 3.24 or newer.

## Tests

The tests check the binding layer against PLAIN's fixture (`plain-backend.md`).

```bash
cd simpledartble
dart test
```

`lifetime_test.dart` runs each case in an isolate of its own and checks whether it ends by itself.
`programs_test.dart` starts Dart programs of their own in `.dart_tool/simpledartble_test`, a package of its own
because Windows can't replace a library the test process has loaded; they force garbage collection through the VM
service, and check that a program ends cleanly. Both run on desktop only. Run the other tests inside an app on an iOS
simulator, an Android emulator or a Flutter desktop target:

```bash
dart run tool/test_app.dart <device>
```

The app is generated under `build/test_app`, depends on both packages, builds PLAIN, and raises the platform minimums
to SimpleBLE's.

## Examples

`examples/simpledartble` is a Dart package of command-line examples, and `examples/simpleflutterble` the SimpleBLE
Explorer app, built with SimpleFlutterBLE. Both depend on the packages by path and use the platform's backend. The app keeps only its Dart code in the
repository: `tool/setup.dart` creates its platform projects with `flutter create` and applies SimpleBLE's minimums and
Bluetooth declarations, and `.gitignore` leaves the projects out. Running it also runs this package's build hook, so the
first run builds SimpleBLE for the host. To try them without hardware, add the
`backend: plain` user-define to the example's pubspec while running them, and leave it out of the repository.

## Publishing

See [Publishing SimpleDartBLE](simpledartble-publishing.md).
