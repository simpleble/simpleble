// Runs the binding tests inside a Flutter app, for the platforms `dart test`
// can't reach. The app is generated under build/test_app and builds SimpleBLE's
// PLAIN backend. Run it from simpledartble/ with a device id from
// `flutter devices`, such as an iOS simulator or an Android emulator:
//
//   dart run tool/test_app.dart <device>
import 'dart:io';

// The test files that run in an app. The others check how a Dart program exits,
// in isolates or programs of their own, and an app's isolate owns the events.
const _tests = ['adapter_test.dart', 'config_test.dart', 'gatt_test.dart', 'local_test.dart', 'logging_test.dart'];

Future<void> main(List<String> arguments) async {
  if (arguments.length != 1) {
    stderr.writeln('Usage: dart run tool/test_app.dart <device>');
    exit(64);
  }

  final package = Directory.fromUri(Platform.script.resolve('../'));
  final app = Directory('${package.path}build/test_app');
  if (!app.existsSync()) {
    await _run('flutter', [
      'create',
      '--platforms=android,ios,macos,linux,windows',
      '--org=org.simpledartble',
      '--project-name=simpledartble_test',
      app.path,
    ]);
    _configure(app);
  }

  File('${app.path}/integration_test/simpledartble_test.dart')
    ..createSync(recursive: true)
    ..writeAsStringSync(
      [
        "import 'package:integration_test/integration_test.dart';",
        "import 'package:test/test.dart';",
        for (final (i, test) in _tests.indexed) "import '../../../test/$test' as test$i;",
        '',
        '// Each file in a group of its own, so its setUpAll() and tearDownAll() run around its own tests.',
        'void main() {',
        '  IntegrationTestWidgetsFlutterBinding.ensureInitialized();',
        for (final (i, test) in _tests.indexed) "  group('$test', test$i.main);",
        '}',
      ].join('\n'),
    );

  await _run('flutter', ['test', 'integration_test', '-d', arguments.single], workingDirectory: app.path);
}

// Points the generated app at both packages and raises its platform minimums to SimpleBLE's.
void _configure(Directory app) {
  _edit('${app.path}/pubspec.yaml', (pubspec) {
    final dependencies = pubspec
        .replaceFirst(
          'dependencies:\n  flutter:\n    sdk: flutter\n',
          'dependencies:\n  flutter:\n    sdk: flutter\n  simpledartble:\n    path: ../..\n'
              '  simpleflutterble:\n    path: ../../../simpleflutterble\n',
        )
        .replaceFirst('dev_dependencies:\n', 'dev_dependencies:\n  integration_test:\n    sdk: flutter\n  test: any\n');
    return '$dependencies\ndependency_overrides:\n  simpledartble:\n    path: ../..\n'
        '\nhooks:\n  user_defines:\n    simpledartble:\n      backend: plain\n';
  });
  _edit('${app.path}/android/app/build.gradle.kts', (gradle) {
    return gradle.replaceFirst('minSdk = flutter.minSdkVersion', 'minSdk = 31');
  });
  _edit('${app.path}/ios/Runner.xcodeproj/project.pbxproj', (project) {
    return project.replaceAll(RegExp(r'IPHONEOS_DEPLOYMENT_TARGET = [0-9.]+;'), 'IPHONEOS_DEPLOYMENT_TARGET = 15.8;');
  });
  _edit('${app.path}/macos/Runner.xcodeproj/project.pbxproj', (project) {
    return project.replaceAll(RegExp(r'MACOSX_DEPLOYMENT_TARGET = [0-9.]+;'), 'MACOSX_DEPLOYMENT_TARGET = 15.0;');
  });
}

void _edit(String path, String Function(String contents) edit) {
  final file = File(path);
  file.writeAsStringSync(edit(file.readAsStringSync()));
}

Future<void> _run(String executable, List<String> arguments, {String? workingDirectory}) async {
  final process = await Process.start(
    executable,
    arguments,
    workingDirectory: workingDirectory,
    mode: ProcessStartMode.inheritStdio,
    runInShell: Platform.isWindows,
  );
  final code = await process.exitCode;
  if (code != 0) exit(code);
}
