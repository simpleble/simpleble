# Publishing SimpleDartBLE and SimpleFlutterBLE

SimpleDartBLE and SimpleFlutterBLE are published to pub.dev with the same version:

```text
simpledartble     The Dart API, and SimpleBLE's native library for each target under prebuilt/
simpleflutterble  Re-exports the API, with the Android module Flutter apps need: initializer, simpledroidbridge
                  classes, permissions
```

Dart programs depend on `simpledartble`, and Flutter apps on `simpleflutterble`. A Flutter plugin is the only way to add
Java code to a Flutter app, and pub installs a package that declares a Flutter SDK constraint only with the Flutter SDK,
so the Android module can't live in `simpledartble`. `simpleflutterble` depends on the exact version of
`simpledartble`, as the module and the native library call each other.

## Native libraries

The published `simpledartble` has no SimpleBLE sources beside it, so its build hook takes the target's library from
`prebuilt/<target>/`. `tool/prebuild.dart` builds them with the hook's CMake configuration
(`lib/src/internal/native_library.dart`, as pub.dev only accepts `build.dart` and `link.dart` in `hook/`). Run it from
`simpledartble/` with the targets the machine builds:

```bash
dart run tool/prebuild.dart macos-arm64 macos-x64
```

| Targets | Built on | Notes |
| --- | --- | --- |
| `android-arm`, `android-arm64`, `android-x64` | Linux | NDK from `ANDROID_NDK_HOME`, debug information stripped |
| `ios-iphoneos-arm64`, `ios-iphonesimulator-arm64`, `ios-iphonesimulator-x64` | macOS | |
| `linux-arm64`, `linux-x64` | manylinux_2_28 containers | Needs glibc 2.28 and the system's `libdbus-1.so.3` |
| `macos-arm64`, `macos-x64` | macOS | |
| `windows-arm64`, `windows-x64` | Windows | Static CRT |

PLAIN libraries are never published, and the hook refuses `backend: plain` outside a checkout.

## Preparing the packages

`dart pub publish` has no prepack step. `tool/prepublish.dart` checks that `prebuilt/` holds every target, then copies
in `LICENSE` for both packages and the simpledroidbridge classes for `simpleflutterble/android/bridge/`.
`.gitignore` leaves them out of the repository, and each package's `.pubignore` replaces it for the archive. The
Android module compiles the repository's `simpledroidbridge` when it's there, and `bridge/` otherwise.
`simpleflutterble/pubspec_overrides.yaml` resolves `simpledartble` from the repository and isn't published.

```bash
cd simpledartble
dart run tool/prepublish.dart
dart pub publish --dry-run
cd ../simpleflutterble
flutter pub publish --dry-run
```

## Publishing

pub.dev accepts uploads from GitHub Actions only for tag pushes, and never deletes a version, so only stable releases
are published. Pushing the release tag `vX.Y.Z` starts `publish_simpledartble.yml`, which runs
`ci_simpledartble.yml` for the tag: it builds every target's library, then prepares, validates and packs both
packages. The publish job then uploads `simpledartble` and `simpleflutterble` with
GitHub's OIDC token, skipping a package whose version is already on pub.dev.

`CI Main` runs the same build for every push and release, and keeps the packed packages as the `simpledartble-pub`
artifact, `simpledartble-<version>.tar.gz`, without publishing them.

### pub.dev setup

Automated publishing can only be turned on for a package that exists, so the first version of each package is
uploaded by hand, from the `simpledartble-pub` artifact of the release's tag run:

```bash
tar -xzf simpledartble-X.Y.Z.tar.gz
(cd simpledartble && dart pub publish)
(cd simpleflutterble && flutter pub publish)
```

That run's publish job fails, as pub.dev doesn't know the packages yet. Then, on each package's **Admin** tab on
pub.dev, enable publishing from GitHub Actions for the repository `simpleble/simpleble` with the tag pattern
`v{{version}}`. Later releases publish from their tag.

Never reuse a published version: pub.dev rejects it.
