# Publishing SimpleSharpBLE

SimpleSharpBLE is published by `ci_main.yml`:

| Event | Package version | Destination |
| --- | --- | --- |
| Push to `main` or manual CI Main run on `main` | `1.1.1-dev.123` | nuget.org |
| Published GitHub release | `1.1.1` | nuget.org and GitHub release assets |
| Pull request or other branch | `1.1.1-dev.123` | CI artifacts |

Versions above are examples. `.github/actions/setup-version` reads the root
`VERSION` and appends `-devN` using the commit count since the preceding tag.
The .NET project reads that file directly and converts the package suffix to
`-dev.N`, so NuGet sorts development builds numerically. Stable versions are
unchanged. Continue using the repository's usual version bump and release process.

CI publishes the built `SimpleSharpBLE-nuget` artifact. It contains the managed
assembly with its XML documentation and the native libraries for Windows
x64/ARM64, Apple Silicon macOS, Linux x64/ARM64, and Android ARM64/x64 under
`runtimes/`, plus static libraries for iOS, the iOS simulator, and Mac Catalyst
under `buildTransitive/apple/`. The packaging job reuses the SimpleCBLE
artifacts from `ci_simplecble.yml`; installed Linux `.so` files keep their
versioned SONAME filenames. Repeated pushes of an existing version are skipped.
`SimpleSharpBLE.Plain` is available for local development.

To check a package locally, stage the native libraries in the same layout as
the CI `Stage SimpleCBLE libraries` step and pack with
`-p:NativeLibrariesDirectory=<staging directory>`. Pass a throwaway
`-p:PackageVersion` and restore consumers with a scratch `NUGET_PACKAGES`
directory, so the local package never shadows a published version in the
global NuGet cache.

## Install a development version

```sh
dotnet add package SimpleSharpBLE --prerelease
```

To select a particular build, use `--version 1.1.1-dev.123`.
