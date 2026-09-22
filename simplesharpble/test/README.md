# Managed binding tests

Build with PLAIN and run the existing tests:

```sh
cmake -S simplesharpble/test/native -B build_sharp_tests -DCMAKE_BUILD_TYPE=Release
cmake --build build_sharp_tests --config Release --parallel 4
dotnet test simplesharpble/test -c Release -p:NativeLibraryDirectory="$PWD/build_sharp_tests/lib"
```

The NativeAOT smoke test analyzes the binding and runs one async scan with a callback:

```sh
dotnet publish simplesharpble/test/aot -c Release -r osx-arm64 \
  -p:NativeLibraryDirectory="$PWD/build_sharp_tests/lib" -o build_sharp_aot
./build_sharp_aot/SimpleSharpBLE.AotTests
```

For a trimmed run, add `-p:PublishAot=false -p:PublishTrimmed=true --self-contained true`.
On Windows, native DLLs are in `build_sharp_tests/bin/Release`.
