# SimpleJSBLE development and publishing

The Node.js addon lives in `simplejsble/src/` (C++, Node-API through
node-addon-api) and the public API in `simplejsble/lib/` (TypeScript, compiled
to `simplejsble/dist/`). The addon links SimpleBLE statically. Tests live in
`simplejsble/test/`, and `simplejsble/test/types/` checks the published
declarations.

## Local builds and tests

From `simplejsble/`, with Node.js 22 or newer and CMake 3.21 or newer:

```sh
npm ci
npm run build:plain   # addon against the PLAIN backend, into prebuilds/<platform>-<arch>
npm test
npm run test:types
npm run build         # addon against the platform's backends
npm run check:prebuild
```

Both builds write the same `prebuilds/<platform>-<arch>/simplejsble.node`, so
rebuild with `build:plain` before running the tests again. The tests only cover
the binding layer and use PLAIN's documented fixture (see
[plain-backend.md](plain-backend.md)).

`check:prebuild` checks what a build can lose without failing: the glibc 2.28
baseline on Linux, the macOS 15.0 deployment target, and on Windows the static
C++ runtime and a delay-loaded `node.exe`, which Electron needs.

## CI distribution

`.github/workflows/ci_simplejsble.yml` runs from `ci_main.yml`:

1. `build` runs the PLAIN tests, then builds and checks the addon on each target:
   Linux x64 and arm64 in `manylinux_2_28` containers, macOS x64 and arm64, and
   Windows x64 and arm64.
2. `package` puts the six addons into `prebuilds/` and runs `npm pack`. The
   `prepack` script refuses to pack without all six, copies the repository's
   `LICENSE.md`, and compiles the TypeScript. The tarball is then installed in
   an empty project and loaded with `require` and `import`.

## Versions and publication

`setup-version` writes the build's version into `simplejsble/package.json`.
`publish-simplejsble` in `ci_main.yml` publishes the packed tarball:

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
rather than `ci_simplejsble.yml`. A new configuration expires unless a publish
succeeds within two days, so set it up shortly before the first run on `main`.
Once publishing works, Settings > Publishing access can disallow tokens.
