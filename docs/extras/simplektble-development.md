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
└── gradle/libs.versions.toml     Dependency versions
```

Targets: `android`, `jvm`, `iosArm64`, `iosSimulatorArm64`, and `macosArm64`.

The library is built with Kotlin 2.3, the oldest version applications can use. Kotlin/Native refuses klibs made by
a newer compiler, so building with a newer Kotlin raises that minimum for iOS and macOS. Change it on purpose.

## How a call reaches SimpleBLE

`Adapter` and `Peripheral` in `commonMain` call the internal `Native` object. `Native` is declared once with `expect`
and implemented twice. Adapters and peripherals are passed around as `Long` IDs that the native side assigns, and the
shared classes keep one Kotlin object per ID.

- **JNI (Android and the JVM).** `Native` is a set of `external` functions implemented in `cpp/simplektble.cpp`.
  `NativeCache` maps the IDs to SimpleBLE objects. SimpleBLE callbacks go to `ThreadRunner`, which calls the Kotlin
  callback interfaces from one thread attached to the JVM. Each platform supplies `loadNativeLibrary()`: Android
  uses `System.loadLibrary`, and the JVM copies the library for its OS and architecture out of the jar.
- **C API (iOS and macOS).** `appleMain` calls SimpleCBLE through cinterop. SimpleCBLE returns a new handle every
  time, so each adapter and peripheral is identified by the SimpleBLE object behind it: the first handle is kept and
  later ones are released. Callbacks receive the ID as userdata and are queued onto one coroutine, in order.

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
- **Apple.** `buildSimpleCBle<Target>` builds SimpleCBLE and SimpleBLE as static libraries for each target, and the
  cinterop klib embeds them along with the frameworks they need. The Apple targets build only on macOS.

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

See [the PLAIN backend notes](plain-backend.md) for the simulated adapter and peripheral the tests use, and the
[publishing guide](simplektble-publishing.md) for releases.
