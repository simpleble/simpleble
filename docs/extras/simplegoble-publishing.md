# SimpleGoBLE development and publishing

The public Go API lives at the root of `simplegoble/`. Native bindings, handle
ownership, callback dispatch, and platform loaders live in
`simplegoble/internal/bindings/`, following the public/internal split used by
the other bindings. All Go tests live in `simplegoble/tests/`.
Runnable examples live in `examples/simplegoble/`.
User guides live in `docs/content/docs/simplegoble/`. Keep API contracts in Go
declaration comments so `go doc` and pkg.go.dev use the same reference.

## Local builds and tests

Build and install a PLAIN SimpleCBLE library from the repository root:

```sh
cmake -S simplecble -B build_simplegoble_plain -DSIMPLEBLE_PLAIN=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build_simplegoble_plain --config Release --parallel
cmake --install build_simplegoble_plain --config Release --prefix "$PWD/install_simplegoble_plain"
```

Test the binding:

```sh
cd simplegoble
go test ./...
go vet ./...
```

Native integration tests find the platform's PLAIN library in
`install_simplegoble_plain/` automatically. Without that installation, they are
skipped. Hardware testing is a separate check.

CI generates the native bundle for distribution. The source examples use a
local module replacement for compilation; run the published examples as shown
in the [examples guide](../content/docs/simplegoble/examples.mdx).

## CI distribution

`ci_main.yml` runs the SimpleCBLE build before calling `ci_simplegoble.yml`.
The Go workflow shows the packaging operations directly:

1. Run binding tests against PLAIN on desktop hosts.
2. Copy binding sources, the README, license, version, source revision, and examples.
3. Read the six desktop SimpleCBLE artifacts, copy shared libraries as regular
   files under `internal/bindings/native/<target>/`, and generate one `go:embed`
   source file per target in `internal/bindings/`.
4. Test the packaged module, including extraction and a native library call.
   Compile its library, examples, and tests for every platform included in the
   bundle. Source examples are covered by these packaged builds.
5. Upload a Go module ZIP with the standard `<module>@<version>/` path prefix.

The examples' development `go.mod` and local replacement are excluded from the
distribution. Their source is copied under the published module's `examples/`
directory, using the published module itself.

## Versions and publication

The existing `setup-version` action reads the root `VERSION`. Main builds use
`X.Y.Z-dev.N`; a published SimpleBLE GitHub release uses `X.Y.Z`. The corresponding
Go tags are `vX.Y.Z-dev.N` and `vX.Y.Z`. Major versions 2 and above add Go's
`/vN` module and import suffix during packaging.

After the main release gate succeeds, `publish-simplegoble` in `ci_main.yml`
updates the generated [simpleble/simplegoble](https://github.com/simpleble/simplegoble)
repository using `SIMPLEGOBLE_DEPLOY_KEY`. Publication updates the distribution
commit and its tag in one atomic push. An identical existing tag is a no-op;
different contents for an existing tag fail the job. Never replace a published
Go module version.
