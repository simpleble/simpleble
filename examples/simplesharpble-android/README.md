# Android scan

Requires .NET 10 with the Android workload, JDK 17, and Android SDK/NDK 29.
Run on Android 12 or newer (ARM64 or x64) with Bluetooth enabled.

From the repository root, build for an ARM64 device:

```sh
cmake -S simplecble -B build_android -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_HOME/ndk/29.0.14206865/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-31
cmake --build build_android --parallel
cmake --install build_android --strip --prefix "$PWD/build_android/install"
mkdir -p build_android/native/android-arm64/native
cp build_android/install/lib/*.so build_android/native/android-arm64/native/
dotnet build examples/simplesharpble-android -t:Install -r android-arm64 \
  "-p:NativeLibrariesDirectory=$PWD/build_android/native"
```

For x64, use `x86_64` as the ABI and `android-x64` as the runtime and staging directory.
Open **SimpleSharpBLE Explorer**, grant Nearby devices permission, and tap **Scan**.

This example references the source tree. Apps using the NuGet package receive the
native libraries and Java bridge automatically; set `SupportedOSPlatformVersion`
to `31.0` or higher and select `android-arm64` and/or `android-x64` as the runtimes.
The scan manifest uses `neverForLocation`, which can filter some BLE beacons.
Peripheral hosting also requires `BLUETOOTH_ADVERTISE` permission.
