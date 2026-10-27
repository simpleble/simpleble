# SimpleRnBLE development and publishing

SimpleRnBLE is a [Nitro module](https://nitro.margelo.com). The pieces live in
`simplernble/`:

- `src/specs/SimpleRnBLE.nitro.ts`: the native objects' interfaces, from which
  `npm run nitrogen` generates the bridging code into `nitrogen/generated/`.
- `cpp/`: the native objects (C++ HybridObjects), shared by iOS and Android.
- `src/`: the public API (TypeScript), compiled to `lib/`.
- `android/`: the Gradle library, its CMake build, `OnLoad.cpp`, and the
  content provider that hands SimpleBLE the application context at startup.
- `SimpleRnBLE.podspec`: the iOS pod, which vendors `prebuilt/<flavor>/SimpleBLE.xcframework`.
- `scripts/`: `build-ios.sh` (the xcframework), `test.mjs` (the binding tests),
  and `prepack.mjs` (packaging).
- `test/`: the binding tests, which run inside the example app.

The example app in `examples/simplernble/` doubles as the test host.
`utils/gradle-react-native/` holds the Gradle wrapper for React Native
projects, since React Native's Gradle plugin needs an older Gradle than
`utils/gradle/`.

`nitrogen/`, `lib/`, `prebuilt/`, `native/`, and `build/` are generated and
ignored by Git.

## Local builds and tests

From `simplernble/`, with Node.js 22 or newer, CMake 3.21 or newer, Xcode and
CocoaPods for iOS, and the Android SDK and NDK for Android:

```sh
npm ci
npm run nitrogen
npm run typecheck
(cd ../examples/simplernble && npm install)
npm run test:android   # on a running emulator; ANDROID_SERIAL picks one
npm run test:ios       # on a booted simulator; SIMULATOR picks one
```

The tests run against SimpleBLE's PLAIN backend and only cover the binding
layer, with PLAIN's documented fixture (see [plain-backend.md](plain-backend.md)).
`test.mjs` builds and installs the example app with the tests in place of its
UI, serves them from a Metro of its own on port 8081, and collects the results
on port 8099.

The flavor is chosen at build time:

- Android: the Gradle property `simplernble.plain=true` builds against PLAIN.
- iOS: `PLAIN=1 scripts/build-ios.sh` builds `prebuilt/plain/`, and
  `SIMPLERNBLE_PLAIN=1 pod install` links it. `test:ios` does both, so run
  `pod install` again without the variable to return the example to
  CoreBluetooth.

## Packaging

```sh
scripts/build-ios.sh
npm pack
```

`build-ios.sh` builds SimpleBLE for iOS devices and simulators into
`prebuilt/release/`. The `prepack` script refuses to pack without it, copies
the SimpleBLE sources the Android build compiles into `native/`, copies the
repository's `LICENSE.md`, and runs `nitrogen` and the TypeScript build.

## CI distribution

`.github/workflows/ci_simplernble.yml` runs from `ci_main.yml`:

1. `ios` checks the types, builds both flavors of the xcframework, runs the
   tests on a simulator, and packs the npm tarball, uploaded as `simplernble-npm`.
2. `android` runs the tests on an emulator, then builds the example against the
   Android backend.

## Versions and publication

`setup-version` writes the build's version into `simplernble/package.json`.
`publish-simplernble` in `ci_main.yml` publishes the packed tarball:

- Pushes to `main` publish `X.Y.Z-dev.N` under the `dev` tag.
- Published GitHub releases publish `X.Y.Z` under `latest`.
- A version that is already on npm is skipped.

Publishing uses npm trusted publishing, so there is no npm token in the
repository secrets. It is configured once on npmjs.com, under the package's
Settings > Trusted publishing:

| Field | Value |
| --- | --- |
| Publisher | GitHub Actions |
| Organization or user | `simpleble` |
| Repository | `simpleble` |
| Workflow filename | `ci_main.yml` |
| Environment | (empty) |
| Allowed actions | `npm publish` |

npm checks the calling workflow, which is why the filename is `ci_main.yml`
rather than `ci_simplernble.yml`. A new configuration expires unless a publish
succeeds within two days, so set it up shortly before the first run on `main`.
Once publishing works, Settings > Publishing access can disallow tokens.
