# Publishing SimpleDroidBLE

Maven Central modules:

```text
org.simpleble:simpledroidbridge:<version>
org.simpleble:simpledroidble:<version>
```

Only the release variants are published. SimpleDroidBLE depends on SimpleDroidBridge at the same version and bundles
the native libraries for all four supported Android ABIs.

## Local verification

Publish the bridge first, then the wrapper, to an isolated local Maven repository:

```bash
repository=$(mktemp -d)
./simpledroidble/gradlew -p simpledroidbridge \
  -Dmaven.repo.local="$repository" publishToMavenLocal
./simpledroidble/gradlew -p simpledroidble \
  -Dmaven.repo.local="$repository" :simpledroidble:publishToMavenLocal
```

## Credentials

CI maps `MAVEN_CENTRAL_USERNAME`, `MAVEN_CENTRAL_PASSWORD`, `MAVEN_SIGNING_KEY`, and
`MAVEN_SIGNING_PASSWORD` to the Vanniktech Gradle properties. Do not store credentials in Gradle files.

## Publishing

Main pushes publish immutable `-devN` versions. Published GitHub releases publish the stable version. The CI job runs
these commands in order:

```bash
./simpledroidble/gradlew -p simpledroidbridge publishAndReleaseToMavenCentral
./simpledroidble/gradlew -p simpledroidble :simpledroidble:publishAndReleaseToMavenCentral
```

Never reuse a version already published to Maven Central.
