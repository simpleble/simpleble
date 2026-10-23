#!/bin/bash
# Points the root Package.swift at a published xcframework archive, for the commit a SwiftPM version tag names.
# Usage: release-manifest.sh <archive URL> <checksum>
set -euo pipefail

MANIFEST="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)/Package.swift"

python3 - "$MANIFEST" "$1" "$2" << 'EOF'
import re
import sys

path, url, checksum = sys.argv[1:]
manifest = open(path).read()

# The flavor selection only applies to the local builds in this repository.
manifest, selections = re.subn(r"// NOTE: In this repository .*?\nlet flavor = .*?\n\n", "", manifest, flags=re.S)
local = 'path: "simpleswiftble/build/\\(flavor)/CSimpleBLE.xcframework"'
targets = manifest.count(local)
manifest = manifest.replace(local, f'url: "{url}", checksum: "{checksum}"')

if selections != 1 or targets != 1 or "flavor" in manifest:
    sys.exit(f"{path} does not have the expected local binary target")
open(path, "w").write(manifest)
EOF
