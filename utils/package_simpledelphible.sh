#!/usr/bin/env bash
# Assembles the SimpleDelphiBLE distribution, which is both the release zip and the contents of the generated
# simpleble/simpledelphible repository that Boss installs from.
# Usage: package_simpledelphible.sh <libraries directory> <output directory>
# The libraries directory has a folder per platform with SimpleCBLE and SimpleBLE built as shared libraries.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
version=$(tr -d ' \t\n\r' < "$repo_root/VERSION")
package_name="simpledelphible-$version"
package_dir="$2/$package_name"

rm -rf "$package_dir" "$2/$package_name.zip"
mkdir -p "$package_dir/lib"
cp -R "$repo_root/simpledelphible/source" "$package_dir/source"
cp -R "$repo_root/examples/simpledelphible" "$package_dir/examples"
cp "$repo_root/simpledelphible/README.md" "$repo_root/LICENSE.md" "$package_dir/"

while read -r platform c_library library; do
    mkdir -p "$package_dir/lib/$platform"
    cp "$1/$platform/$c_library" "$1/$platform/$library" "$package_dir/lib/$platform/"
done <<'PLATFORMS'
win32 simplecble.dll simpleble.dll
win64 simplecble.dll simpleble.dll
macos-arm64 libsimplecble.dylib libsimpleble.dylib
macos-x64 libsimplecble.dylib libsimpleble.dylib
linux-x64 libsimplecble.so libsimpleble.so
linux-arm64 libsimplecble.so libsimpleble.so
PLATFORMS

# Boss resolves versions from the repository's tags and adds mainsrc to the project's search path.
cat > "$package_dir/boss.json" <<EOF
{
  "name": "simpledelphible",
  "description": "SimpleBLE for Delphi and Free Pascal: Bluetooth Low Energy on Windows, macOS, and Linux.",
  "version": "$version",
  "homepage": "https://github.com/simpleble/simpledelphible",
  "mainsrc": "source/",
  "browsingpath": "",
  "projects": [],
  "dependencies": {}
}
EOF

(cd "$2" && zip -qr "$package_name.zip" "$package_name")
echo "$2/$package_name.zip"
