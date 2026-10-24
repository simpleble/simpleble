import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

// Each setting with a way to read it, to change it to a value different from
// the current one, and to reset its group.
final class _Setting {
  _Setting(this.group, this.name, this.read, this.change, this.reset);

  final String group;
  final String name;
  final Object Function() read;
  final void Function(int index) change;
  final void Function() reset;
}

Duration _longer(Duration duration, int index) => duration + Duration(seconds: index + 1);

ConnectionPriorityRequest _other(ConnectionPriorityRequest request) =>
    ConnectionPriorityRequest.values[(request.index + 1) % ConnectionPriorityRequest.values.length];

final _settings = [
  _Setting(
    'simpleBluez',
    'useSystemBus',
    () => Config.simpleBluez.useSystemBus,
    (_) => Config.simpleBluez.useSystemBus = !Config.simpleBluez.useSystemBus,
    Config.simpleBluez.reset,
  ),
  _Setting(
    'simpleBluez',
    'connectionTimeout',
    () => Config.simpleBluez.connectionTimeout,
    (index) => Config.simpleBluez.connectionTimeout = _longer(Config.simpleBluez.connectionTimeout, index),
    Config.simpleBluez.reset,
  ),
  _Setting(
    'simpleBluez',
    'disconnectionTimeout',
    () => Config.simpleBluez.disconnectionTimeout,
    (index) => Config.simpleBluez.disconnectionTimeout = _longer(Config.simpleBluez.disconnectionTimeout, index),
    Config.simpleBluez.reset,
  ),
  _Setting(
    'winRT',
    'experimentalUseOwnMtaApartment',
    // ignore: deprecated_member_use_from_same_package
    () => Config.winRT.experimentalUseOwnMtaApartment,
    // ignore: deprecated_member_use_from_same_package
    (_) => Config.winRT.experimentalUseOwnMtaApartment = !Config.winRT.experimentalUseOwnMtaApartment,
    Config.winRT.reset,
  ),
  _Setting(
    'winRT',
    'experimentalReinitializeWinrtApartmentOnMainThread',
    () => Config.winRT.experimentalReinitializeWinrtApartmentOnMainThread,
    (_) => Config.winRT.experimentalReinitializeWinrtApartmentOnMainThread =
        !Config.winRT.experimentalReinitializeWinrtApartmentOnMainThread,
    Config.winRT.reset,
  ),
  _Setting(
    'winRT',
    'useDeferredDisconnect',
    () => Config.winRT.useDeferredDisconnect,
    (_) => Config.winRT.useDeferredDisconnect = !Config.winRT.useDeferredDisconnect,
    Config.winRT.reset,
  ),
  _Setting(
    'android',
    'negotiateMtu',
    () => Config.android.negotiateMtu,
    (_) => Config.android.negotiateMtu = !Config.android.negotiateMtu,
    Config.android.reset,
  ),
  _Setting(
    'android',
    'connectionPriorityRequest',
    () => Config.android.connectionPriorityRequest,
    (_) => Config.android.connectionPriorityRequest = _other(Config.android.connectionPriorityRequest),
    Config.android.reset,
  ),
  _Setting(
    'dongl',
    'useDonglBackend',
    () => Config.dongl.useDonglBackend,
    (_) => Config.dongl.useDonglBackend = !Config.dongl.useDonglBackend,
    Config.dongl.reset,
  ),
  _Setting(
    'dongl',
    'autoUpdate',
    () => Config.dongl.autoUpdate,
    (_) => Config.dongl.autoUpdate = !Config.dongl.autoUpdate,
    Config.dongl.reset,
  ),
  _Setting(
    'dongl',
    'forceUpdate',
    () => Config.dongl.forceUpdate,
    (_) => Config.dongl.forceUpdate = !Config.dongl.forceUpdate,
    Config.dongl.reset,
  ),
];

Map<String, Object> _snapshot() => {
  for (final setting in _settings) '${setting.group}.${setting.name}': setting.read(),
};

void _changeAll() {
  for (final (index, setting) in _settings.indexed) {
    setting.change(index);
  }
}

void main() {
  tearDown(Config.resetAll);

  test('every setting reaches its own native setting, and each reset() restores only its own group', () {
    final defaults = _snapshot();

    // Changing all of them at once shows a setting wired to the wrong native one.
    _changeAll();
    final changed = _snapshot();
    for (final key in defaults.keys) {
      expect(changed[key], isNot(defaults[key]), reason: key);
    }

    final groups = {for (final setting in _settings) setting.group: setting.reset};
    for (final MapEntry(key: group, value: reset) in groups.entries) {
      Config.resetAll();
      _changeAll();
      reset();
      final after = _snapshot();
      for (final key in defaults.keys) {
        expect(after[key], key.startsWith('$group.') ? defaults[key] : changed[key], reason: '$group.reset() and $key');
      }
    }

    _changeAll();
    Config.resetAll();
    expect(_snapshot(), defaults);
  });
}
