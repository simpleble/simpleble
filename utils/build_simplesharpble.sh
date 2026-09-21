#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="$repo_root/build_simplesharpble_tests"

# The fixture selects the PLAIN backend; these tests do not require Bluetooth.
"${CMAKE:-cmake}" -S "$repo_root/simplesharpble/test/native" -B "$build_directory" -DCMAKE_BUILD_TYPE=Release
"${CMAKE:-cmake}" --build "$build_directory" --parallel 4
"${DOTNET:-dotnet}" test "$repo_root/simplesharpble/test" -c Release \
    "-p:NativeLibraryDirectory=$build_directory/lib"
