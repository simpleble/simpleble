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

CI publishes the built `SimpleSharpBLE-nuget` artifact, with the managed
assembly, Windows x64/ARM64 native DLLs, and Apple Silicon macOS native libraries.
Platform builds feed one packaging job.
Repeated pushes of an existing version are skipped. `SimpleSharpBLE.Plain`
is available for local development.

## Install a development version

```sh
dotnet add package SimpleSharpBLE --prerelease
```

To select a particular build, use `--version 1.1.1-dev.123`.
