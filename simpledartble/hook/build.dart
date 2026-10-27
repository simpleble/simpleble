import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:hooks/hooks.dart';
import 'package:simpledartble/src/internal/native_library.dart';

// Provides SimpleDartBLE's native library. In a checkout of the SimpleBLE
// repository it builds the library with CMake from the sources beside this
// package; the published package carries a prebuilt library for each target.
// The `backend: plain` user-define builds SimpleBLE's PLAIN backend instead of
// the platform's.
void main(List<String> args) async {
  await build(args, (input, output) async {
    if (!input.config.buildCodeAssets) return;

    final code = input.config.code;
    final plain = input.userDefines['backend'] == 'plain';
    final android = code.targetOS == OS.android;
    final library = NativeLibrary(
      code.targetOS,
      code.targetArchitecture,
      iOSSdk: code.targetOS == OS.iOS ? code.iOS.targetSdk : null,
      // NOTE: The compiler is <ndk>/toolchains/llvm/prebuilt/<host>/bin/clang.
      ndk: android ? code.cCompiler?.compiler.resolve('../../../../../') : null,
      androidApi: android && code.android.targetNdkApi > 31 ? code.android.targetNdkApi : 31,
    );

    final repository = input.packageRoot.resolve('../');
    final Uri file;
    if (File.fromUri(repository.resolve('simplecble/CMakeLists.txt')).existsSync()) {
      file = await library.build(
        input.packageRoot.resolve('native/'),
        input.outputDirectoryShared.resolve('${library.target}-${plain ? 'plain' : 'native'}/'),
        plain: plain,
      );
      for (final path in [
        'VERSION',
        'cmake/',
        'dependencies/',
        'simpleble/',
        'simplebluez/',
        'simplecble/',
        'simpledbus/',
        'simpledartble/native/',
      ]) {
        output.dependencies.addAll(_files(repository.resolve(path)));
      }
    } else {
      file = input.packageRoot.resolve('prebuilt/${library.target}/${library.fileName}');
      if (plain || !File.fromUri(file).existsSync()) {
        throw UnsupportedError('SimpleDartBLE has no ${plain ? 'PLAIN ' : ''}library for ${library.target}');
      }
      output.dependencies.add(file);
    }

    output.assets.code.add(
      CodeAsset(package: input.packageName, name: 'simpledartble.dart', linkMode: DynamicLoadingBundled(), file: file),
    );
  });
}

Iterable<Uri> _files(Uri path) {
  final file = File.fromUri(path);
  if (file.existsSync()) return [path];
  return Directory.fromUri(path).listSync(recursive: true).whereType<File>().map((file) => file.uri);
}
