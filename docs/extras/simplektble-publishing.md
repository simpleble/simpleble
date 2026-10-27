# Publishing SimpleKtBLE

SimpleKtBLE is published to Maven Central as one Kotlin Multiplatform library. Applications depend on
`org.simpleble:simplektble:<version>`, and Gradle picks the artifact for each target:

```text
org.simpleble:simplektble                       Root module with the Gradle module metadata
org.simpleble:simplektble-android               Android library
org.simpleble:simplektble-android-native        JNI library for the four Android ABIs, required by the above
org.simpleble:simplektble-jvm                   JVM jar carrying the JNI library of every desktop platform
org.simpleble:simplektble-iosarm64              iOS devices
org.simpleble:simplektble-iossimulatorarm64     iOS simulator
org.simpleble:simplektble-macosarm64            macOS
```

The Apple artifacts embed SimpleBLE as static libraries, so they can only be built on macOS. Every artifact is
published from one macOS build.

The jars carry the license as `META-INF/LICENSE.md`. The Android libraries carry it in their own directory,
`META-INF/<artifact>/LICENSE.md`, because Android apps fail to build when two dependencies ship the same path.

## Native libraries

The jvm jar needs the JNI library of every desktop platform, built from `simplektble/cpp` and collected in this
layout:

```text
jni/
  aarch64/
    libsimplektble-jni.dylib
    libsimplektble-jni.so
  x64/
    libsimplektble-jni.dylib
    libsimplektble-jni.so
    simplektble-jni.dll
```

Pass the directory with `-Psimplektble.jvmNativeLibs=<dir>`. Publishing the jvm artifact to Maven Central runs
`verifyJvmNativeLibraries`, which fails when any of them is missing.

Never publish a build made with `-Psimplektble.plain=true`. The publishing tasks refuse it.

## Local verification

Publish to an isolated local Maven repository:

```bash
repository=$(mktemp -d)
utils/gradle/gradlew -p simplektble -Psimplektble.jvmNativeLibs=jni \
  -Dmaven.repo.local="$repository" :simplektble:verifyJvmNativeLibraries publishToMavenLocal
```

## Credentials

CI maps `MAVEN_CENTRAL_USERNAME`, `MAVEN_CENTRAL_PASSWORD`, `MAVEN_SIGNING_KEY`, and `MAVEN_SIGNING_PASSWORD` to the
Vanniktech Gradle properties. Do not store credentials in Gradle files.

## Publishing

Main pushes publish immutable `-dev.N` versions. Published GitHub releases publish the stable version. The
`ci_simplektble.yml` workflow builds the JNI libraries, and the `publish-simplektble` job in `ci_main.yml` publishes
from macOS:

```bash
utils/gradle/gradlew -p simplektble -Psimplektble.jvmNativeLibs=jni publishAndReleaseToMavenCentral
```

Never reuse a version already published to Maven Central.
