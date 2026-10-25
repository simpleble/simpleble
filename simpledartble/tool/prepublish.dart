// Prepares simpledartble and simpleflutterble for `dart pub publish`. It
// checks that prebuilt/ holds every target's library, from tool/prebuild.dart,
// and copies in what the packages need from the rest of the repository: the
// license, and the simpledroidbridge classes the Android module compiles. Run
// it from simpledartble/:
//
//   dart run tool/prepublish.dart
import 'dart:io';

import '../hook/native_library.dart';

void main() {
  final package = Directory.fromUri(Platform.script.resolve('../')).path;
  final repository = '$package../';
  final flutter = '${repository}simpleflutterble/';

  final missing = [
    for (final target in NativeLibrary.published)
      if (!File('${package}prebuilt/$target/${NativeLibrary.parse(target).fileName}').existsSync()) target,
  ];
  if (missing.isNotEmpty) {
    stderr.writeln('prebuilt/ has no library for ${missing.join(', ')}. Build them with tool/prebuild.dart.');
    exit(1);
  }

  for (final destination in [package, flutter]) {
    File('${repository}LICENSE.md').copySync('${destination}LICENSE');
  }

  final bridge = Directory('${flutter}android/bridge');
  if (bridge.existsSync()) bridge.deleteSync(recursive: true);
  final sources = Directory('${repository}simpledroidbridge/src/main/java');
  for (final source in sources.listSync(recursive: true).whereType<File>()) {
    final copy = File('${bridge.path}/${source.path.substring(sources.path.length + 1)}');
    copy.parent.createSync(recursive: true);
    source.copySync(copy.path);
  }
}
