#!/bin/bash
# Assembles the package that simpleble/simpleswiftble publishes in a directory: the sources, README and license, and a
# Package.swift that uses a published xcframework archive.
# Usage: package.sh <directory> <archive URL> <checksum>
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

mkdir -p "$1"
cp -R "$ROOT/simpleswiftble/Sources" "$ROOT/simpleswiftble/README.md" "$ROOT/LICENSE.md" "$1/"

python3 - "$ROOT/Package.swift" "$1/Package.swift" "$2" "$3" << 'EOF'
import re
import sys

source, target, url, checksum = sys.argv[1:]
manifest = open(source).read()

# The flavor selection and the tests only apply to the builds in this repository.
manifest, selections = re.subn(r"// NOTE: In this repository .*?\nlet flavor = .*?\n\n", "", manifest, flags=re.S)
manifest, tests = re.subn(r"\n        \.testTarget\(.*?\n        \),", "", manifest, flags=re.S)
local = 'path: "simpleswiftble/build/\\(flavor)/CSimpleBLE.xcframework"'
binaries = manifest.count(local)
manifest = manifest.replace(local, f'url: "{url}", checksum: "{checksum}"')
# The published repository holds the sources at its root.
sources = manifest.count('path: "simpleswiftble/Sources/SimpleSwiftBLE"')
manifest = manifest.replace('path: "simpleswiftble/Sources/SimpleSwiftBLE"', 'path: "Sources/SimpleSwiftBLE"')

if (selections, tests, binaries, sources) != (1, 1, 1, 1) or "flavor" in manifest or "\"simpleswiftble/" in manifest:
    sys.exit(f"{source} does not have the expected targets")
open(target, "w").write(manifest)
EOF
