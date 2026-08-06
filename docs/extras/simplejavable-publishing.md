# Publishing SimpleJavaBLE

SimpleJavaBLE is published as `org.simpleble:simplejavable:<version>`. The main JAR bundles the supported native
libraries so consumers only need one dependency.

## Prerequisites

1. Create and verify an account in the [Central Publisher Portal](https://central.sonatype.com/).
2. Verify the `org.simpleble` namespace using the DNS record for `simpleble.org`.
3. Generate a Central Portal user token.
4. Create a GPG signing key and publish its public key to a supported keyserver.
5. Build all native targets and collect them in this layout:

   ```text
   jni/
     aarch64/
       libsimplejavable.dylib
       libsimplejavable.so
     x64/
       libsimplejavable.dylib
       libsimplejavable.so
       simplejavable.dll
   ```

## Credentials

Use these environment variables locally, or map protected CI secrets to them:

```text
ORG_GRADLE_PROJECT_mavenCentralUsername
ORG_GRADLE_PROJECT_mavenCentralPassword
ORG_GRADLE_PROJECT_signingInMemoryKey
ORG_GRADLE_PROJECT_signingInMemoryKeyPassword
```

The Central username and password are the generated token values, not the Portal login. The signing key is the
ASCII-armored private key. `ORG_GRADLE_PROJECT_signingInMemoryKeyId` is optional.

Recommended GitHub secret names are `MAVEN_CENTRAL_USERNAME`, `MAVEN_CENTRAL_PASSWORD`, `MAVEN_SIGNING_KEY`, and
`MAVEN_SIGNING_PASSWORD`.

## Verification

From the repository root, publish to the local Maven cache without contacting Central or requiring credentials if
needed for troubleshooting:

```bash
utils/gradle/gradlew -p simplejavable/java clean publishToMavenLocal \
  -PnativeLibPath=/path/to/jni
```

Verify the version, POM, artifacts, license, and complete native bundle:

```bash
utils/gradle/gradlew -p simplejavable/java verifyReleasePublication \
  -PnativeLibPath=/path/to/jni
```

## Publishing

CI publishes and releases `-devN` versions from `main`. Publishing a GitHub release publishes and releases the stable
version. Maven Central versions are immutable, so every development build must have a unique suffix.

To run the same signed Central Portal flow manually:

```bash
utils/gradle/gradlew -p simplejavable/java clean publishAndReleaseToMavenCentral \
  -PnativeLibPath=/path/to/jni
```

Do not reuse a version that has already been published.
