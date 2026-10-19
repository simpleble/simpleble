# SimpleKtBLE development

This is contributor documentation for SimpleKtBLE, the Kotlin Multiplatform bindings. It is kept outside
`content/docs` and is not included in the published documentation site.

## Layout

```text
simplektble/
├── simplektble/                  Kotlin Multiplatform library, package org.simplektble
│   └── src/
│       ├── commonMain/           Public API, written once for every platform
│       ├── jniMain/              Native as JNI functions, shared by Android and the JVM
│       ├── androidMain/          Library loading, permission helpers, manifest
│       ├── jvmMain/              Library loading from the jar
│       ├── appleMain/            Native over the SimpleCBLE C API, for iOS and macOS
│       ├── nativeInterop/        cinterop definition for SimpleCBLE
│       └── commonTest/           Tests every target runs, against PLAIN
├── simplektble-android-native/   Android library that builds and ships the JNI library
├── cpp/                          JNI layer over SimpleBLE (libsimplektble-jni)
│   └── apple/                    Functions SimpleCBLE lacks, built with it for Apple targets
└── gradle/libs.versions.toml     Dependency versions
```

Targets: `android`, `jvm`, `iosArm64`, `iosSimulatorArm64`, and `macosArm64`.

The library is built with Kotlin 2.3, the oldest version applications can use. Kotlin/Native refuses klibs made by
a newer compiler, so building with a newer Kotlin raises that minimum for iOS and macOS. Change it on purpose.

## How a call reaches SimpleBLE

`Adapter`, `Peripheral` and the local GATT classes in `commonMain` call the internal `Native` object. `Native` is
declared once with `expect` and implemented twice. Objects are passed around as `Long` IDs that the native side
assigns, and the shared classes keep one Kotlin object per ID. Backends are named by their identifier instead, since
SimpleBLE has one backend per identifier.

An adapter's or peripheral's ID is the address of SimpleBLE's own object behind it, read through `cpp/Internal.h` the
way SimpleJSBLE does. SimpleBLE keeps one such object per adapter and per device, so the same device always comes
back as the same Kotlin object. `underlying()` cannot stand in for it: it returns the operating system's object, which
the Dongl and simulated backends do not have. Local GATT objects are numbered as they are created.

Properties return state that is already known, and anything that waits on the Bluetooth stack or can fail at run
time on iOS and macOS is a function. Kotlin/Native lets only functions declare `@Throws`, and an undeclared exception
that reaches Swift ends the app. On Android, reading a name or an address without `BLUETOOTH_CONNECT` still throws,
which only Kotlin code sees.

Every flow buffers without limit, so each event reaches each active collector in order. A disconnection ends, in
both bridges, the subscriptions the peripheral had at that moment, queued after their last values: a flow does not
watch `onDisconnected`, which could still hold a disconnection from before a reconnection. A read handler that throws
or is missing serves the stored value, as in SimpleJavaBLE.

- **JNI (Android and the JVM).** `Native` is a set of `external` functions implemented in `cpp/simplektble.cpp`.
  `NativeCache` maps the IDs to SimpleBLE objects. SimpleBLE callbacks go to `ThreadRunner`, which calls the Kotlin
  callback interfaces from one thread attached to the JVM. Read handlers and Dongl pairing handlers instead run on the
  SimpleBLE thread that waits for their answer. Every thread attaches as a daemon (`cpp/Jvm.h`), since a desktop JVM
  waits for its non-daemon threads before exiting. Each platform supplies `loadNativeLibrary()`: Android uses
  `System.loadLibrary`, and the JVM copies the library for its OS and architecture out of the jar.
- **Android context.** Hosting a GATT server needs the application context. An androidx.startup initializer keeps
  it when the app starts, and `createLocalPeripheral()` passes it to SimpleBLE.
- **Android shrinking.** The JNI library and the SimpleBLE Android backend look classes and methods up by name, so
  both Android libraries publish consumer keep rules (`consumer-rules.pro`) for apps that run R8.
- **C API (iOS and macOS).** `appleMain` calls SimpleCBLE through cinterop. SimpleCBLE returns a new handle every
  time and has no identity function, so `cpp/apple` adds `simplektble_adapter_identity` and
  `simplektble_peripheral_identity`: the first handle for an ID is kept and later ones are released. Lists come from
  SimpleCBLE's list handles, read once and released after each entry is interned. SimpleCBLE's advanced functions are
  named differently on iOS and macOS and exist only with the CoreBluetooth backend, so `cpp/apple` also provides the
  cached peripherals (`simplektble/snapshot.h`) and the advertised local name, and reports `OPERATION_NOT_SUPPORTED` without that backend, as the JNI layer does. Callbacks
  receive the ID as userdata and are queued onto one coroutine, in order; read and pairing handlers run on the calling
  thread. Their failures go to SimpleBLE's logger through `simplektble/logging.h`, since SimpleCBLE cannot log a
  message.

`ThreadRunner` and `NativeCache` are never destroyed. On the JVM, static destructors run at exit, when the callback
thread can no longer detach, and joining it hangs the process.

## Native builds

Gradle runs CMake for every native library, with separate build directories under
`simplektble/build/native/` for PLAIN and the real backends:

- **Android.** `simplektble-android-native` builds `cpp/` with the NDK for four ABIs. It is a separate module because
  the Android plugin for Kotlin Multiplatform cannot build native code. It also compiles the `simpledroidbridge`
  classes from `simpledroidbridge/`, which the SimpleBLE Android backend calls into: one Gradle build cannot mix
  Android Gradle plugin versions, so the bridge's own build cannot be included.
- **JVM.** `buildJvmNative` builds `cpp/` for the host, and the jar carries it as `native/<arch>/<library>`. To
  package every platform, build the libraries elsewhere and pass a directory laid out as `<arch>/<library>` with
  `-Psimplektble.jvmNativeLibs=<dir>`. The library links only `libjvm`: nothing in it uses AWT, and Java
  runtimes built without the `java.desktop` module have no `libjawt` to load.
- **Apple.** `buildSimpleCBle<Target>` builds `cpp/apple`, which includes SimpleCBLE and SimpleBLE, as static
  libraries for each target, and the cinterop klib embeds them along with the frameworks they need. The Apple targets
  build only on macOS.

## Building and testing

Run from the repository root with the shared wrapper:

```bash
utils/gradle/gradlew -p simplektble :simplektble:assemble
```

The tests in `commonTest` run on every target, with the native libraries built against PLAIN:

```bash
utils/gradle/gradlew -p simplektble -Psimplektble.plain=true \
  :simplektble:jvmTest :simplektble:macosArm64Test :simplektble:iosSimulatorArm64Test
```

On Android they run as device tests, so they need an emulator or device:

```bash
utils/gradle/gradlew -p simplektble :simplektble:connectedAndroidDeviceTest -Psimplektble.plain=true
```

The examples in `examples/simplektble` build SimpleKtBLE from this checkout:

```bash
utils/gradle/gradlew -p examples/simplektble runScan
```

See [the PLAIN backend notes](plain-backend.md) for the simulated adapter and peripheral the tests use, and the
[publishing guide](simplektble-publishing.md) for releases.
