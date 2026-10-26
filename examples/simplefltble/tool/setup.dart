// Creates SimpleBLE Explorer's Android, iOS, macOS, Windows and Linux projects
// with `flutter create`, raises their minimums to SimpleBLE's and declares its
// Bluetooth use. Run it once from examples/simplefltble:
//
//   dart run tool/setup.dart
import 'dart:io';

const _usage = 'SimpleBLE Explorer scans for and connects to Bluetooth devices.';

Future<void> main() async {
  final app = Directory.fromUri(Platform.script.resolve('../')).path;

  // NOTE: --empty leaves out the template's widget test and keeps the app's own files.
  final result = await Process.run(
    'flutter',
    [
      'create',
      '--empty',
      '--platforms=android,ios,macos,windows,linux',
      '--org=org.simpleble',
      '--project-name=simplefltble_explorer',
      '.',
    ],
    workingDirectory: app,
    runInShell: Platform.isWindows,
  );
  if (result.exitCode != 0) {
    stderr.write(result.stdout);
    stderr.write(result.stderr);
    exit(result.exitCode);
  }

  _edit(
    '${app}android/app/build.gradle.kts',
    (text) => text.replaceFirst('minSdk = flutter.minSdkVersion', 'minSdk = 31'),
  );
  _edit('${app}ios/Runner.xcodeproj/project.pbxproj', (text) => _setting(text, 'IPHONEOS_DEPLOYMENT_TARGET', '15.8'));
  _edit('${app}macos/Runner.xcodeproj/project.pbxproj', (text) => _setting(text, 'MACOSX_DEPLOYMENT_TARGET', '15.0'));
  for (final plist in ['ios/Runner/Info.plist', 'macos/Runner/Info.plist']) {
    _edit('$app$plist', (text) => _entry(text, 'NSBluetoothAlwaysUsageDescription', '<string>$_usage</string>'));
  }
  for (final entitlements in ['macos/Runner/DebugProfile.entitlements', 'macos/Runner/Release.entitlements']) {
    _edit('$app$entitlements', (text) => _entry(text, 'com.apple.security.device.bluetooth', '<true/>'));
  }
  stdout.writeln('Created the platform projects. Run the app with `flutter run`.');
}

String _setting(String project, String name, String value) =>
    project.replaceAll(RegExp('$name = [0-9.]+;'), '$name = $value;');

// Adds a key to a property list's top-level dictionary, unless it's there.
String _entry(String plist, String key, String value) =>
    plist.contains('<key>$key</key>') ? plist : plist.replaceFirst('<dict>\n', '<dict>\n\t<key>$key</key>\n\t$value\n');

void _edit(String path, String Function(String text) edit) {
  final file = File(path);
  file.writeAsStringSync(edit(file.readAsStringSync()));
}
