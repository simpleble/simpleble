#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="$repo_root/build_simpledelphible_tests"

# The tests use the PLAIN backend and Free Pascal; they do not require Bluetooth.
"${CMAKE:-cmake}" -S "$repo_root/simplecble" -B "$build_directory/native" \
    -DSIMPLEBLE_PLAIN=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
"${CMAKE:-cmake}" --build "$build_directory/native" --parallel 4

mkdir -p "$build_directory/tests"
"${FPC:-fpc}" -O1 -vewn \
    -Fu"$repo_root/simpledelphible/source" -Fi"$repo_root/simpledelphible/source" \
    -Fl"$build_directory/native/lib" -k-rpath -k"$build_directory/native/lib" \
    -FU"$build_directory/tests" -FE"$build_directory/tests" \
    "$repo_root/simpledelphible/tests/SimpleDelphiBLETests.dpr"
"$build_directory/tests/SimpleDelphiBLETests"
