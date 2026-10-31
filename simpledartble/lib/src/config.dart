import 'internal/bindings.g.dart';

/// Process-wide settings of each backend. Set them before the first call that
/// reaches a backend; later changes might not take effect.
abstract final class Config {
  static const simpleBluez = SimpleBluezConfig._();
  static const winRT = WinRTConfig._();
  static const coreBluetooth = CoreBluetoothConfig._();
  static const android = AndroidConfig._();
  static const dongl = DonglConfig._();

  /// Restores every backend's defaults.
  static void resetAll() => simpleble_config_reset_all();
}

/// Settings of the Linux backend, which reaches BlueZ over D-Bus.
final class SimpleBluezConfig {
  const SimpleBluezConfig._();

  /// Reach BlueZ over the D-Bus system bus (true) or the session bus.
  bool get useSystemBus => simpleble_config_simplebluez_get_use_system_bus();
  set useSystemBus(bool value) => simpleble_config_simplebluez_set_use_system_bus(value);

  /// How long `connect()` waits for BlueZ, in whole milliseconds.
  Duration get connectionTimeout => Duration(milliseconds: simpleble_config_simplebluez_get_connection_timeout_ms());
  set connectionTimeout(Duration value) => simpleble_config_simplebluez_set_connection_timeout_ms(value.inMilliseconds);

  /// How long `disconnect()` waits for BlueZ, in whole milliseconds.
  Duration get disconnectionTimeout =>
      Duration(milliseconds: simpleble_config_simplebluez_get_disconnection_timeout_ms());
  set disconnectionTimeout(Duration value) =>
      simpleble_config_simplebluez_set_disconnection_timeout_ms(value.inMilliseconds);

  /// Register SimpleBLE's BlueZ pairing agent as KeyboardDisplay and as
  /// BlueZ's default agent, so that passkey and numeric comparison requests
  /// reach the adapters' pairing handlers, such as
  /// `Adapter.setNumericComparisonHandler`. Off by default, which allows only
  /// Just Works pairing. Read once, when the backend starts.
  bool get interactivePairing => simpleble_config_simplebluez_get_interactive_pairing();
  set interactivePairing(bool value) => simpleble_config_simplebluez_set_interactive_pairing(value);

  void reset() => simpleble_config_simplebluez_reset();
}

/// Settings of the Windows backend.
final class WinRTConfig {
  const WinRTConfig._();

  @Deprecated('SimpleBLE uses its own WinRT MTA apartment by default.')
  bool get experimentalUseOwnMtaApartment => simpleble_config_winrt_get_experimental_use_own_mta_apartment();
  @Deprecated('SimpleBLE uses its own WinRT MTA apartment by default.')
  set experimentalUseOwnMtaApartment(bool value) =>
      simpleble_config_winrt_set_experimental_use_own_mta_apartment(value);

  bool get experimentalReinitializeWinrtApartmentOnMainThread =>
      simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread();
  set experimentalReinitializeWinrtApartmentOnMainThread(bool value) =>
      simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(value);

  bool get useDeferredDisconnect => simpleble_config_winrt_get_use_deferred_disconnect();
  set useDeferredDisconnect(bool value) => simpleble_config_winrt_set_use_deferred_disconnect(value);

  void reset() => simpleble_config_winrt_reset();
}

/// Settings of the iOS and macOS backend.
final class CoreBluetoothConfig {
  const CoreBluetoothConfig._();

  void reset() => simpleble_config_corebluetooth_reset();
}

/// The connection priorities of Android's `BluetoothGatt.requestConnectionPriority()`.
enum ConnectionPriorityRequest {
  /// Asks for nothing.
  disabled,
  balanced,
  high,
  lowPower,
  dck,
}

/// Settings of the Android backend.
final class AndroidConfig {
  const AndroidConfig._();

  /// Negotiate the MTU on connect.
  bool get negotiateMtu => simpleble_config_android_get_negotiate_mtu();
  set negotiateMtu(bool value) => simpleble_config_android_set_negotiate_mtu(value);

  /// What each connection asks Android for once connected.
  ConnectionPriorityRequest get connectionPriorityRequest =>
      ConnectionPriorityRequest.values[simpleble_config_android_get_connection_priority().value + 1];
  set connectionPriorityRequest(ConnectionPriorityRequest value) => simpleble_config_android_set_connection_priority(
    simpleble_config_android_connection_priority_t.fromValue(value.index - 1),
  );

  void reset() => simpleble_config_android_reset();
}

/// Settings of the Dongl backend.
final class DonglConfig {
  const DonglConfig._();

  /// Include Dongl adapters in `Backend.getBackends()` and `Adapter.getAdapters()`.
  bool get useDonglBackend => simpleble_config_dongl_get_use_dongl_backend();
  set useDonglBackend(bool value) => simpleble_config_dongl_set_use_dongl_backend(value);

  bool get autoUpdate => simpleble_config_dongl_get_auto_update();
  set autoUpdate(bool value) => simpleble_config_dongl_set_auto_update(value);

  bool get forceUpdate => simpleble_config_dongl_get_force_update();
  set forceUpdate(bool value) => simpleble_config_dongl_set_force_update(value);

  void reset() => simpleble_config_dongl_reset();
}
