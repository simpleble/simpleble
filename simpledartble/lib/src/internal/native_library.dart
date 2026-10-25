import 'dart:io';

import 'package:code_assets/code_assets.dart';

/// SimpleDartBLE's native library for one target, built with CMake from the
/// SimpleBLE sources beside this package. The build hook and tool/prebuild.dart
/// share it, so the published libraries match the ones a checkout builds.
///
/// NOTE: pub.dev only accepts build.dart and link.dart in hook/, so this lives
/// with the package's internal code, which the hook imports.
final class NativeLibrary {
  NativeLibrary(this.os, this.architecture, {this.iOSSdk, this.ndk, this.androidApi = 31});

  /// The library for a target name, such as `macos-arm64` or `ios-iphonesimulator-x64`.
  factory NativeLibrary.parse(String target, {Uri? ndk}) {
    final parts = target.split('-');
    final os = OS.fromString(parts.first);
    return NativeLibrary(
      os,
      Architecture.fromString(parts.last),
      iOSSdk: os == OS.iOS ? IOSSdk.fromString(parts[1]) : null,
      ndk: ndk,
    );
  }

  /// The targets the published package carries a library for.
  static const published = [
    'android-arm',
    'android-arm64',
    'android-x64',
    'ios-iphoneos-arm64',
    'ios-iphonesimulator-arm64',
    'ios-iphonesimulator-x64',
    'linux-arm64',
    'linux-x64',
    'macos-arm64',
    'macos-x64',
    'windows-arm64',
    'windows-x64',
  ];

  final OS os;
  final Architecture architecture;
  final IOSSdk? iOSSdk;

  /// The Android NDK, which Android targets build with.
  final Uri? ndk;
  final int androidApi;

  String get target => os == OS.iOS ? 'ios-$iOSSdk-$architecture' : '$os-$architecture';

  String get fileName => switch (os) {
    OS.windows => 'simpledartble.dll',
    OS.macOS || OS.iOS => 'libsimpledartble.dylib',
    _ => 'libsimpledartble.so',
  };

  /// Builds the library from [sources], the package's native/ folder, in
  /// [buildDirectory], and returns the library's path.
  Future<Uri> build(Uri sources, Uri buildDirectory, {required bool plain}) async {
    await _run('cmake', [
      '-S',
      sources.toFilePath(),
      '-B',
      buildDirectory.toFilePath(),
      '-DCMAKE_BUILD_TYPE=Release',
      '-DSIMPLEBLE_PLAIN=${plain ? 'ON' : 'OFF'}',
      '-DDART_SDK_INCLUDE_DIR=${File(Platform.resolvedExecutable).parent.parent.uri.resolve('include/').toFilePath()}',
      ..._configuration(),
    ]);
    await _run('cmake', ['--build', buildDirectory.toFilePath(), '--config', 'Release', '--parallel']);
    return buildDirectory.resolve(os == OS.windows ? 'bin/Release/$fileName' : 'lib/$fileName');
  }

  List<String> _configuration() {
    final generator = _onPath('ninja') ? ['-G', 'Ninja'] : <String>[];
    switch (os) {
      case OS.macOS:
        return [
          ...generator,
          '-DCMAKE_OSX_ARCHITECTURES=${_appleArchitecture()}',
          '-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
        ];
      case OS.iOS:
        return [
          ...generator,
          '-DCMAKE_SYSTEM_NAME=iOS',
          '-DCMAKE_OSX_SYSROOT=$iOSSdk',
          '-DCMAKE_OSX_ARCHITECTURES=${_appleArchitecture()}',
          '-DCMAKE_OSX_DEPLOYMENT_TARGET=15.8',
        ];
      case OS.android:
        final ndk = this.ndk;
        if (ndk == null) throw StateError('Building SimpleDartBLE for Android needs the Android NDK');
        return [
          ...generator,
          '-DCMAKE_TOOLCHAIN_FILE=${ndk.resolve('build/cmake/android.toolchain.cmake').toFilePath()}',
          '-DANDROID_ABI=${_androidAbi()}',
          '-DANDROID_PLATFORM=android-$androidApi',
          '-DANDROID_STL=c++_static',
        ];
      case OS.windows:
        return [
          '-A',
          architecture == Architecture.arm64 ? 'ARM64' : 'x64',
          '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
        ];
      case OS.linux:
        return generator;
      default:
        throw UnsupportedError('SimpleDartBLE does not support $os');
    }
  }

  String _appleArchitecture() => switch (architecture) {
    Architecture.arm64 => 'arm64',
    Architecture.x64 => 'x86_64',
    _ => throw UnsupportedError('SimpleDartBLE does not support $architecture'),
  };

  String _androidAbi() => switch (architecture) {
    Architecture.arm64 => 'arm64-v8a',
    Architecture.arm => 'armeabi-v7a',
    Architecture.x64 => 'x86_64',
    _ => throw UnsupportedError('SimpleDartBLE does not support $architecture on Android'),
  };
}

bool _onPath(String executable) {
  final separator = Platform.isWindows ? ';' : ':';
  final names = Platform.isWindows ? ['$executable.exe'] : [executable];
  return (Platform.environment['PATH'] ?? '')
      .split(separator)
      .any((directory) => names.any((name) => File('$directory${Platform.pathSeparator}$name').existsSync()));
}

Future<void> _run(String executable, List<String> arguments) async {
  final result = await Process.run(executable, arguments, runInShell: Platform.isWindows);
  if (result.exitCode != 0) {
    throw ProcessException(executable, arguments, '${result.stdout}\n${result.stderr}', result.exitCode);
  }
}
