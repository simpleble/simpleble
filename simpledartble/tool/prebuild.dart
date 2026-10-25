// Builds the native libraries the published package carries, into
// prebuilt/<target>/. Run it from simpledartble/ with the targets this machine
// builds, such as macos-arm64 or android-arm64; Android targets build with the
// NDK in ANDROID_NDK_HOME:
//
//   dart run tool/prebuild.dart <target>...
import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:simpledartble/src/internal/native_library.dart';

Future<void> main(List<String> targets) async {
  if (targets.isEmpty || !targets.every(NativeLibrary.published.contains)) {
    stderr.writeln('Usage: dart run tool/prebuild.dart <target>...');
    stderr.writeln('Targets: ${NativeLibrary.published.join(' ')}');
    exit(64);
  }

  final package = Directory.fromUri(Platform.script.resolve('../')).uri;
  final ndk = Platform.environment['ANDROID_NDK_HOME'];
  for (final target in targets) {
    final library = NativeLibrary.parse(target, ndk: ndk == null ? null : Directory(ndk).uri);
    final built = await library.build(
      package.resolve('native/'),
      package.resolve('build/prebuild/$target/'),
      plain: false,
    );
    final prebuilt = File.fromUri(package.resolve('prebuilt/$target/${library.fileName}'));
    prebuilt.parent.createSync(recursive: true);
    File.fromUri(built).copySync(prebuilt.path);
    // NOTE: The NDK always builds with debug information, which is most of the library's size.
    if (library.os == OS.android) await _strip(library.ndk!, prebuilt.path);
    stdout.writeln('Built ${prebuilt.path}');
  }
}

Future<void> _strip(Uri ndk, String path) async {
  final hosts = Directory.fromUri(ndk.resolve('toolchains/llvm/prebuilt/')).listSync().whereType<Directory>();
  final strip = '${hosts.single.path}/bin/llvm-strip${Platform.isWindows ? '.exe' : ''}';
  final result = await Process.run(strip, ['--strip-debug', path]);
  if (result.exitCode != 0) throw ProcessException(strip, [path], '${result.stderr}', result.exitCode);
}
