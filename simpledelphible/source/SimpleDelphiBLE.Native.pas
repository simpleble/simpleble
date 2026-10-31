// Declarations of the SimpleCBLE C API (simplecble/include/simplecble), as the C headers define them. The functions
// are bound when LoadSimpleCBLE loads the library.
unit SimpleDelphiBLE.Native;

{$I SimpleDelphiBLE.inc}

{$IFDEF FPC}
  {$PACKRECORDS C}
{$ENDIF}

interface

const
{$IF DEFINED(MSWINDOWS)}
  SimpleCBLELibraryName = 'simplecble.dll';
{$ELSEIF DEFINED(DARWIN) or DEFINED(MACOS)}
  SimpleCBLELibraryName = 'libsimplecble.dylib';
{$ELSE}
  SimpleCBLELibraryName = 'libsimplecble.so';
{$ENDIF}

  SIMPLEBLE_UUID_STR_LEN = 37;

type
  // NOTE: C enums are int-sized, while Pascal enums default to one byte, so they're declared as integers.
  simpleble_err_t = Int32;
  simpleble_os_t = Int32;
  simpleble_address_type_t = Int32;
  simpleble_log_level_t = Int32;
  simpleble_config_android_connection_priority_t = Int32;

  // A simpleble_error_t*, the opaque error a call reports through its out_error parameter.
  psimpleble_error_t = Pointer;

  simpleble_backend_t = Pointer;
  simpleble_adapter_t = Pointer;
  simpleble_peripheral_t = Pointer;
  simpleble_local_peripheral_t = Pointer;
  simpleble_local_service_t = Pointer;
  simpleble_local_characteristic_t = Pointer;

  simpleble_backend_list_t = Pointer;
  simpleble_adapter_list_t = Pointer;
  simpleble_peripheral_list_t = Pointer;
  simpleble_service_list_t = Pointer;
  simpleble_manufacturer_data_list_t = Pointer;
  simpleble_local_service_list_t = Pointer;
  simpleble_local_characteristic_list_t = Pointer;

  simpleble_uuid_t = record
    value: array[0..SIMPLEBLE_UUID_STR_LEN - 1] of AnsiChar;
  end;

  simpleble_descriptor_t = record
    uuid: simpleble_uuid_t;
  end;
  psimpleble_descriptor_t = ^simpleble_descriptor_t;

  simpleble_characteristic_t = record
    uuid: simpleble_uuid_t;
    can_read: Boolean;
    can_write_request: Boolean;
    can_write_command: Boolean;
    can_notify: Boolean;
    can_indicate: Boolean;
    descriptor_count: NativeUInt;
    descriptors: psimpleble_descriptor_t;
  end;
  psimpleble_characteristic_t = ^simpleble_characteristic_t;

  simpleble_service_t = record
    uuid: simpleble_uuid_t;
    data_length: NativeUInt;
    data: PByte;
    characteristic_count: NativeUInt;
    characteristics: psimpleble_characteristic_t;
  end;

  simpleble_manufacturer_data_t = record
    manufacturer_id: UInt16;
    data_length: NativeUInt;
    data: PByte;
  end;

  simpleble_adapter_callback_t = procedure(adapter: simpleble_adapter_t; userdata: Pointer); cdecl;
  simpleble_adapter_peripheral_callback_t = procedure(adapter: simpleble_adapter_t; peripheral: simpleble_peripheral_t;
    userdata: Pointer); cdecl;
  simpleble_peripheral_callback_t = procedure(peripheral: simpleble_peripheral_t; userdata: Pointer); cdecl;
  simpleble_peripheral_value_callback_t = procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; userdata: Pointer); cdecl;
  simpleble_dongl_passkey_request_callback_t = function(handle: simpleble_peripheral_t; passkey: PAnsiChar;
    userdata: Pointer): Boolean; cdecl;
  simpleble_dongl_passkey_display_callback_t = procedure(handle: simpleble_peripheral_t; passkey: PAnsiChar;
    userdata: Pointer); cdecl;
  simpleble_dongl_numeric_comparison_callback_t = function(handle: simpleble_peripheral_t; passkey: PAnsiChar;
    userdata: Pointer): Boolean; cdecl;
{$IFDEF SIMPLEDELPHIBLE_LINUX}
  simpleble_linux_passkey_request_callback_t = function(handle: simpleble_adapter_t; peer: PAnsiChar;
    passkey: PAnsiChar; userdata: Pointer): Boolean; cdecl;
  simpleble_linux_passkey_display_callback_t = procedure(handle: simpleble_adapter_t; peer: PAnsiChar;
    passkey: PAnsiChar; userdata: Pointer); cdecl;
  simpleble_linux_numeric_comparison_callback_t = function(handle: simpleble_adapter_t; peer: PAnsiChar;
    passkey: PAnsiChar; userdata: Pointer): Boolean; cdecl;
  simpleble_linux_pairing_authorization_callback_t = function(handle: simpleble_adapter_t; peer: PAnsiChar;
    userdata: Pointer): Boolean; cdecl;
{$ENDIF}
  simpleble_local_peripheral_client_callback_t = procedure(handle: simpleble_local_peripheral_t;
    client_address: PAnsiChar; userdata: Pointer); cdecl;
  simpleble_local_characteristic_read_callback_t = function(handle: simpleble_local_characteristic_t;
    out data_length: NativeUInt; userdata: Pointer): PByte; cdecl;
  simpleble_local_characteristic_write_callback_t = procedure(handle: simpleble_local_characteristic_t; data: PByte;
    data_length: NativeUInt; userdata: Pointer); cdecl;
  simpleble_local_characteristic_callback_t = procedure(handle: simpleble_local_characteristic_t;
    userdata: Pointer); cdecl;
  simpleble_log_callback_t = procedure(level: simpleble_log_level_t; module: PAnsiChar; file_name: PAnsiChar;
    line: UInt32; function_name: PAnsiChar; message: PAnsiChar); cdecl;

var
  // simplecble.h
  simpleble_free: procedure(handle: Pointer); cdecl;

  // error.h
  simpleble_error_code: function(error: psimpleble_error_t): simpleble_err_t; cdecl;
  simpleble_error_message: function(error: psimpleble_error_t): PAnsiChar; cdecl;
  simpleble_error_release: procedure(var error: psimpleble_error_t); cdecl;

  // utils.h
  simpleble_get_operating_system: function: simpleble_os_t; cdecl;
  simpleble_get_version: function: PAnsiChar; cdecl;

  // adapter.h
  simpleble_adapter_is_bluetooth_enabled: function(var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_adapter_get_adapters: function(out out_list: simpleble_adapter_list_t;
    var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_adapter_list_get: function(list: simpleble_adapter_list_t; index: NativeUInt;
    var out_error: psimpleble_error_t): simpleble_adapter_t; cdecl;
  simpleble_adapter_list_release: procedure(list: simpleble_adapter_list_t); cdecl;
  simpleble_adapter_release_handle: procedure(handle: simpleble_adapter_t); cdecl;
  simpleble_adapter_identity: function(handle: simpleble_adapter_t): Pointer; cdecl;
  simpleble_adapter_identifier: function(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): PAnsiChar;
    cdecl;
  simpleble_adapter_address: function(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): PAnsiChar; cdecl;
  simpleble_adapter_power_on: procedure(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_adapter_power_off: procedure(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_adapter_is_powered: function(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): Boolean;
    cdecl;
  simpleble_adapter_set_callback_on_power_on: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl;
  simpleble_adapter_set_callback_on_power_off: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl;
  simpleble_adapter_scan_start: procedure(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_adapter_scan_stop: procedure(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_adapter_scan_is_active: function(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): Boolean;
    cdecl;
  simpleble_adapter_scan_for: procedure(handle: simpleble_adapter_t; timeout_ms: Int32;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_adapter_scan_get_results: function(handle: simpleble_adapter_t; out out_list: simpleble_peripheral_list_t;
    var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_adapter_get_paired_peripherals: function(handle: simpleble_adapter_t;
    out out_list: simpleble_peripheral_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_adapter_get_connected_peripherals: function(handle: simpleble_adapter_t;
    out out_list: simpleble_peripheral_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_adapter_set_callback_on_scan_start: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl;
  simpleble_adapter_set_callback_on_scan_stop: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl;
  simpleble_adapter_set_callback_on_scan_updated: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_peripheral_callback_t; userdata: Pointer); cdecl;
  simpleble_adapter_set_callback_on_scan_found: procedure(handle: simpleble_adapter_t;
    callback: simpleble_adapter_peripheral_callback_t; userdata: Pointer); cdecl;

  // peripheral.h
  simpleble_peripheral_release_handle: procedure(handle: simpleble_peripheral_t); cdecl;
  simpleble_peripheral_identity: function(handle: simpleble_peripheral_t): Pointer; cdecl;
  simpleble_peripheral_list_get: function(list: simpleble_peripheral_list_t; index: NativeUInt;
    var out_error: psimpleble_error_t): simpleble_peripheral_t; cdecl;
  simpleble_peripheral_list_release: procedure(list: simpleble_peripheral_list_t); cdecl;
  simpleble_peripheral_identifier: function(handle: simpleble_peripheral_t;
    var out_error: psimpleble_error_t): PAnsiChar; cdecl;
  simpleble_peripheral_address: function(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): PAnsiChar;
    cdecl;
  simpleble_peripheral_address_type: function(handle: simpleble_peripheral_t;
    var out_error: psimpleble_error_t): simpleble_address_type_t; cdecl;
  simpleble_peripheral_rssi: function(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Int16; cdecl;
  simpleble_peripheral_tx_power: function(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Int16;
    cdecl;
  simpleble_peripheral_mtu: function(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): UInt16; cdecl;
  simpleble_peripheral_is_connected: function(handle: simpleble_peripheral_t;
    var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_peripheral_is_connectable: function(handle: simpleble_peripheral_t;
    var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_peripheral_manufacturer_data: function(handle: simpleble_peripheral_t;
    out out_list: simpleble_manufacturer_data_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_manufacturer_data_list_get: procedure(list: simpleble_manufacturer_data_list_t; index: NativeUInt;
    var out_data: simpleble_manufacturer_data_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_manufacturer_data_list_release: procedure(list: simpleble_manufacturer_data_list_t); cdecl;
  simpleble_manufacturer_data_release: procedure(var data: simpleble_manufacturer_data_t); cdecl;
  simpleble_peripheral_connect: procedure(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_disconnect: procedure(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_is_paired: function(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Boolean;
    cdecl;
  simpleble_peripheral_unpair: procedure(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_services: function(handle: simpleble_peripheral_t; out out_list: simpleble_service_list_t;
    var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_service_list_get: procedure(list: simpleble_service_list_t; index: NativeUInt;
    var out_service: simpleble_service_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_service_list_release: procedure(list: simpleble_service_list_t); cdecl;
  simpleble_service_release: procedure(var service: simpleble_service_t); cdecl;
  simpleble_peripheral_read: function(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; out data_length: NativeUInt; var out_error: psimpleble_error_t): PByte; cdecl;
  simpleble_peripheral_write_request: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_write_command: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_notify: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; callback: simpleble_peripheral_value_callback_t; userdata: Pointer;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_indicate: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; callback: simpleble_peripheral_value_callback_t; userdata: Pointer;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_unsubscribe: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_read_descriptor: function(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; descriptor: simpleble_uuid_t; out data_length: NativeUInt;
    var out_error: psimpleble_error_t): PByte; cdecl;
  simpleble_peripheral_write_descriptor: procedure(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
    characteristic: simpleble_uuid_t; descriptor: simpleble_uuid_t; data: PByte; data_length: NativeUInt;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_peripheral_set_callback_on_connected: procedure(handle: simpleble_peripheral_t;
    callback: simpleble_peripheral_callback_t; userdata: Pointer); cdecl;
  simpleble_peripheral_set_callback_on_disconnected: procedure(handle: simpleble_peripheral_t;
    callback: simpleble_peripheral_callback_t; userdata: Pointer); cdecl;

  // advanced.h, the parts every build has
  simpleble_advanced_dongl_set_passkey_request_callback: procedure(handle: simpleble_peripheral_t;
    callback: simpleble_dongl_passkey_request_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_dongl_set_passkey_display_callback: procedure(handle: simpleble_peripheral_t;
    callback: simpleble_dongl_passkey_display_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_dongl_set_numeric_comparison_callback: procedure(handle: simpleble_peripheral_t;
    callback: simpleble_dongl_numeric_comparison_callback_t; userdata: Pointer; var out_error: psimpleble_error_t);
    cdecl;
  simpleble_advanced_dongl_secure_connection: procedure(handle: simpleble_peripheral_t; mitm: Boolean;
    var out_error: psimpleble_error_t); cdecl;

{$IFDEF SIMPLEDELPHIBLE_MACOS}
  // advanced.h, macOS builds
  simpleble_advanced_macos_set_advertisement_local_name: procedure(handle: simpleble_local_peripheral_t;
    local_name: PAnsiChar; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_macos_retrieve_cached_peripheral: function(handle: simpleble_adapter_t; identifier: PAnsiChar;
    var out_error: psimpleble_error_t): simpleble_peripheral_t; cdecl;
{$ENDIF}

{$IFDEF SIMPLEDELPHIBLE_LINUX}
  // advanced.h, Linux builds
  simpleble_advanced_linux_set_advertisement_local_name: procedure(handle: simpleble_local_peripheral_t;
    local_name: PAnsiChar; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_linux_set_passkey_request_callback: procedure(handle: simpleble_adapter_t;
    callback: simpleble_linux_passkey_request_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_linux_set_passkey_display_callback: procedure(handle: simpleble_adapter_t;
    callback: simpleble_linux_passkey_display_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  simpleble_advanced_linux_set_numeric_comparison_callback: procedure(handle: simpleble_adapter_t;
    callback: simpleble_linux_numeric_comparison_callback_t; userdata: Pointer; var out_error: psimpleble_error_t);
    cdecl;
  simpleble_advanced_linux_set_pairing_authorization_callback: procedure(handle: simpleble_adapter_t;
    callback: simpleble_linux_pairing_authorization_callback_t; userdata: Pointer;
    var out_error: psimpleble_error_t); cdecl;
{$ENDIF}

  // backend.h
  simpleble_backend_get_backends: function(out out_list: simpleble_backend_list_t;
    var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_backend_list_get: function(list: simpleble_backend_list_t; index: NativeUInt;
    var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  simpleble_backend_list_release: procedure(list: simpleble_backend_list_t); cdecl;
  simpleble_backend_get_native: function(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  simpleble_backend_get_dongl: function(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  simpleble_backend_get_simulator: function(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  simpleble_backend_release_handle: procedure(handle: simpleble_backend_t); cdecl;
  simpleble_backend_identifier: function(handle: simpleble_backend_t; var out_error: psimpleble_error_t): PAnsiChar;
    cdecl;
  simpleble_backend_is_bluetooth_enabled: function(handle: simpleble_backend_t;
    var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_backend_get_adapters: function(handle: simpleble_backend_t; out out_list: simpleble_adapter_list_t;
    var out_error: psimpleble_error_t): NativeUInt; cdecl;

  // adapter.h, local peripherals
  simpleble_adapter_create_local_peripheral: function(handle: simpleble_adapter_t;
    var out_error: psimpleble_error_t): simpleble_local_peripheral_t; cdecl;

  // local/peripheral.h
  simpleble_local_peripheral_release_handle: procedure(handle: simpleble_local_peripheral_t); cdecl;
  simpleble_local_peripheral_add_advertised_service: procedure(handle: simpleble_local_peripheral_t;
    service: simpleble_uuid_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_local_peripheral_set_advertised_manufacturer_data: procedure(handle: simpleble_local_peripheral_t;
    company_id: UInt16; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  simpleble_local_peripheral_set_connectable: procedure(handle: simpleble_local_peripheral_t; connectable: Boolean;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_local_peripheral_add_service: function(handle: simpleble_local_peripheral_t; uuid: simpleble_uuid_t;
    var out_error: psimpleble_error_t): simpleble_local_service_t; cdecl;
  simpleble_local_peripheral_services: function(handle: simpleble_local_peripheral_t;
    out out_list: simpleble_local_service_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_local_peripheral_remove_all_services: procedure(handle: simpleble_local_peripheral_t;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_local_peripheral_start: procedure(handle: simpleble_local_peripheral_t; var out_error: psimpleble_error_t);
    cdecl;
  simpleble_local_peripheral_stop: procedure(handle: simpleble_local_peripheral_t; var out_error: psimpleble_error_t);
    cdecl;
  simpleble_local_peripheral_is_started: function(handle: simpleble_local_peripheral_t;
    var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_local_peripheral_is_advertising: function(handle: simpleble_local_peripheral_t;
    var out_error: psimpleble_error_t): Boolean; cdecl;
  simpleble_local_peripheral_set_callback_on_client_connected: procedure(handle: simpleble_local_peripheral_t;
    callback: simpleble_local_peripheral_client_callback_t; userdata: Pointer); cdecl;
  simpleble_local_peripheral_set_callback_on_client_disconnected: procedure(handle: simpleble_local_peripheral_t;
    callback: simpleble_local_peripheral_client_callback_t; userdata: Pointer); cdecl;

  // local/service.h
  simpleble_local_service_release_handle: procedure(handle: simpleble_local_service_t); cdecl;
  simpleble_local_service_identity: function(handle: simpleble_local_service_t): Pointer; cdecl;
  simpleble_local_service_list_get: function(list: simpleble_local_service_list_t; index: NativeUInt;
    var out_error: psimpleble_error_t): simpleble_local_service_t; cdecl;
  simpleble_local_service_list_release: procedure(list: simpleble_local_service_list_t); cdecl;
  simpleble_local_service_characteristics: function(handle: simpleble_local_service_t;
    out out_list: simpleble_local_characteristic_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  simpleble_local_service_uuid: procedure(handle: simpleble_local_service_t; out out_uuid: simpleble_uuid_t;
    var out_error: psimpleble_error_t); cdecl;
  simpleble_local_service_add_characteristic: function(handle: simpleble_local_service_t; uuid: simpleble_uuid_t;
    capabilities: UInt32; var out_error: psimpleble_error_t): simpleble_local_characteristic_t; cdecl;

  // local/characteristic.h
  simpleble_local_characteristic_release_handle: procedure(handle: simpleble_local_characteristic_t); cdecl;
  simpleble_local_characteristic_identity: function(handle: simpleble_local_characteristic_t): Pointer; cdecl;
  simpleble_local_characteristic_list_get: function(list: simpleble_local_characteristic_list_t; index: NativeUInt;
    var out_error: psimpleble_error_t): simpleble_local_characteristic_t; cdecl;
  simpleble_local_characteristic_list_release: procedure(list: simpleble_local_characteristic_list_t); cdecl;
  simpleble_local_characteristic_uuid: procedure(handle: simpleble_local_characteristic_t;
    out out_uuid: simpleble_uuid_t; var out_error: psimpleble_error_t); cdecl;
  simpleble_local_characteristic_capabilities: function(handle: simpleble_local_characteristic_t;
    var out_error: psimpleble_error_t): UInt32; cdecl;
  simpleble_local_characteristic_value: function(handle: simpleble_local_characteristic_t; out data_length: NativeUInt;
    var out_error: psimpleble_error_t): PByte; cdecl;
  simpleble_local_characteristic_set_value: procedure(handle: simpleble_local_characteristic_t; data: PByte;
    data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  simpleble_local_characteristic_set_callback_on_read: procedure(handle: simpleble_local_characteristic_t;
    callback: simpleble_local_characteristic_read_callback_t; userdata: Pointer); cdecl;
  simpleble_local_characteristic_set_callback_on_write: procedure(handle: simpleble_local_characteristic_t;
    callback: simpleble_local_characteristic_write_callback_t; userdata: Pointer); cdecl;
  simpleble_local_characteristic_set_callback_on_subscribed: procedure(handle: simpleble_local_characteristic_t;
    callback: simpleble_local_characteristic_callback_t; userdata: Pointer); cdecl;
  simpleble_local_characteristic_set_callback_on_unsubscribed: procedure(handle: simpleble_local_characteristic_t;
    callback: simpleble_local_characteristic_callback_t; userdata: Pointer); cdecl;

  // config.h
  simpleble_config_reset_all: procedure; cdecl;
  simpleble_config_simplebluez_reset: procedure; cdecl;
  simpleble_config_simplebluez_get_use_system_bus: function: Boolean; cdecl;
  simpleble_config_simplebluez_set_use_system_bus: procedure(enabled: Boolean); cdecl;
  simpleble_config_simplebluez_get_connection_timeout_ms: function: Int64; cdecl;
  simpleble_config_simplebluez_set_connection_timeout_ms: procedure(timeout_ms: Int64); cdecl;
  simpleble_config_simplebluez_get_disconnection_timeout_ms: function: Int64; cdecl;
  simpleble_config_simplebluez_set_disconnection_timeout_ms: procedure(timeout_ms: Int64); cdecl;
  simpleble_config_simplebluez_get_interactive_pairing: function: Boolean; cdecl;
  simpleble_config_simplebluez_set_interactive_pairing: procedure(enabled: Boolean); cdecl;
  simpleble_config_winrt_reset: procedure; cdecl;
  simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread: function: Boolean; cdecl;
  simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread: procedure(enabled: Boolean);
    cdecl;
  simpleble_config_winrt_get_use_deferred_disconnect: function: Boolean; cdecl;
  simpleble_config_winrt_set_use_deferred_disconnect: procedure(enabled: Boolean); cdecl;
  simpleble_config_corebluetooth_reset: procedure; cdecl;
  simpleble_config_android_reset: procedure; cdecl;
  simpleble_config_android_get_negotiate_mtu: function: Boolean; cdecl;
  simpleble_config_android_set_negotiate_mtu: procedure(enabled: Boolean); cdecl;
  simpleble_config_android_get_connection_priority: function: simpleble_config_android_connection_priority_t; cdecl;
  simpleble_config_android_set_connection_priority: procedure(
    priority: simpleble_config_android_connection_priority_t); cdecl;
  simpleble_config_dongl_reset: procedure; cdecl;
  simpleble_config_dongl_get_use_dongl_backend: function: Boolean; cdecl;
  simpleble_config_dongl_set_use_dongl_backend: procedure(enabled: Boolean); cdecl;
  simpleble_config_dongl_get_auto_update: function: Boolean; cdecl;
  simpleble_config_dongl_set_auto_update: procedure(enabled: Boolean); cdecl;
  simpleble_config_dongl_get_force_update: function: Boolean; cdecl;
  simpleble_config_dongl_set_force_update: procedure(enabled: Boolean); cdecl;

  // logging.h
  simpleble_logging_set_level: procedure(level: simpleble_log_level_t); cdecl;
  simpleble_logging_get_level: function: simpleble_log_level_t; cdecl;
  simpleble_logging_set_callback: procedure(callback: simpleble_log_callback_t); cdecl;
  simpleble_logging_has_callback: function: Boolean; cdecl;
  simpleble_logging_log_default_stdout: procedure; cdecl;
  simpleble_logging_log_default_file: procedure; cdecl;
  simpleble_logging_log_default_file_path: procedure(path: PAnsiChar); cdecl;

// Loads the library from FileName and binds every function. On failure, Error says why.
function LoadSimpleCBLE(const FileName: string; out Error: string): Boolean;

implementation

uses
{$IFDEF FPC}
  dynlibs;
{$ELSE}
  {$IFDEF MSWINDOWS}Winapi.Windows,{$ENDIF} System.SysUtils;

type
  TLibHandle = HMODULE;

const
  NilHandle = 0;

function GetProcedureAddress(Lib: TLibHandle; Name: PAnsiChar): Pointer;
begin
{$IFDEF MSWINDOWS}
  Result := GetProcAddress(Lib, Name);
{$ELSE}
  Result := GetProcAddress(Lib, PChar(string(Name)));
{$ENDIF}
end;
{$ENDIF}

function LoadSimpleCBLE(const FileName: string; out Error: string): Boolean;
var
  Lib: TLibHandle;
  Missing: string;

  procedure Bind(var Proc; Name: PAnsiChar);
  begin
    Pointer(Proc) := GetProcedureAddress(Lib, Name);
    if (Pointer(Proc) = nil) and (Missing = '') then
      Missing := string(Name);
  end;

begin
{$IFDEF FPC}
  Lib := LoadLibrary(FileName);
{$ELSE}
  Lib := SafeLoadLibrary(FileName);
{$ENDIF}
  if Lib = NilHandle then
  begin
    Error := FileName + ' could not be loaded';
    Exit(False);
  end;
  Missing := '';
  Bind(simpleble_free, 'simpleble_free');
  Bind(simpleble_error_code, 'simpleble_error_code');
  Bind(simpleble_error_message, 'simpleble_error_message');
  Bind(simpleble_error_release, 'simpleble_error_release');
  Bind(simpleble_get_operating_system, 'simpleble_get_operating_system');
  Bind(simpleble_get_version, 'simpleble_get_version');
  Bind(simpleble_adapter_is_bluetooth_enabled, 'simpleble_adapter_is_bluetooth_enabled');
  Bind(simpleble_adapter_get_adapters, 'simpleble_adapter_get_adapters');
  Bind(simpleble_adapter_list_get, 'simpleble_adapter_list_get');
  Bind(simpleble_adapter_list_release, 'simpleble_adapter_list_release');
  Bind(simpleble_adapter_release_handle, 'simpleble_adapter_release_handle');
  Bind(simpleble_adapter_identity, 'simpleble_adapter_identity');
  Bind(simpleble_adapter_identifier, 'simpleble_adapter_identifier');
  Bind(simpleble_adapter_address, 'simpleble_adapter_address');
  Bind(simpleble_adapter_power_on, 'simpleble_adapter_power_on');
  Bind(simpleble_adapter_power_off, 'simpleble_adapter_power_off');
  Bind(simpleble_adapter_is_powered, 'simpleble_adapter_is_powered');
  Bind(simpleble_adapter_set_callback_on_power_on, 'simpleble_adapter_set_callback_on_power_on');
  Bind(simpleble_adapter_set_callback_on_power_off, 'simpleble_adapter_set_callback_on_power_off');
  Bind(simpleble_adapter_scan_start, 'simpleble_adapter_scan_start');
  Bind(simpleble_adapter_scan_stop, 'simpleble_adapter_scan_stop');
  Bind(simpleble_adapter_scan_is_active, 'simpleble_adapter_scan_is_active');
  Bind(simpleble_adapter_scan_for, 'simpleble_adapter_scan_for');
  Bind(simpleble_adapter_scan_get_results, 'simpleble_adapter_scan_get_results');
  Bind(simpleble_adapter_get_paired_peripherals, 'simpleble_adapter_get_paired_peripherals');
  Bind(simpleble_adapter_get_connected_peripherals, 'simpleble_adapter_get_connected_peripherals');
  Bind(simpleble_adapter_set_callback_on_scan_start, 'simpleble_adapter_set_callback_on_scan_start');
  Bind(simpleble_adapter_set_callback_on_scan_stop, 'simpleble_adapter_set_callback_on_scan_stop');
  Bind(simpleble_adapter_set_callback_on_scan_updated, 'simpleble_adapter_set_callback_on_scan_updated');
  Bind(simpleble_adapter_set_callback_on_scan_found, 'simpleble_adapter_set_callback_on_scan_found');
  Bind(simpleble_peripheral_release_handle, 'simpleble_peripheral_release_handle');
  Bind(simpleble_peripheral_identity, 'simpleble_peripheral_identity');
  Bind(simpleble_peripheral_list_get, 'simpleble_peripheral_list_get');
  Bind(simpleble_peripheral_list_release, 'simpleble_peripheral_list_release');
  Bind(simpleble_peripheral_identifier, 'simpleble_peripheral_identifier');
  Bind(simpleble_peripheral_address, 'simpleble_peripheral_address');
  Bind(simpleble_peripheral_address_type, 'simpleble_peripheral_address_type');
  Bind(simpleble_peripheral_rssi, 'simpleble_peripheral_rssi');
  Bind(simpleble_peripheral_tx_power, 'simpleble_peripheral_tx_power');
  Bind(simpleble_peripheral_mtu, 'simpleble_peripheral_mtu');
  Bind(simpleble_peripheral_is_connected, 'simpleble_peripheral_is_connected');
  Bind(simpleble_peripheral_is_connectable, 'simpleble_peripheral_is_connectable');
  Bind(simpleble_peripheral_manufacturer_data, 'simpleble_peripheral_manufacturer_data');
  Bind(simpleble_manufacturer_data_list_get, 'simpleble_manufacturer_data_list_get');
  Bind(simpleble_manufacturer_data_list_release, 'simpleble_manufacturer_data_list_release');
  Bind(simpleble_manufacturer_data_release, 'simpleble_manufacturer_data_release');
  Bind(simpleble_peripheral_connect, 'simpleble_peripheral_connect');
  Bind(simpleble_peripheral_disconnect, 'simpleble_peripheral_disconnect');
  Bind(simpleble_peripheral_is_paired, 'simpleble_peripheral_is_paired');
  Bind(simpleble_peripheral_unpair, 'simpleble_peripheral_unpair');
  Bind(simpleble_peripheral_services, 'simpleble_peripheral_services');
  Bind(simpleble_service_list_get, 'simpleble_service_list_get');
  Bind(simpleble_service_list_release, 'simpleble_service_list_release');
  Bind(simpleble_service_release, 'simpleble_service_release');
  Bind(simpleble_peripheral_read, 'simpleble_peripheral_read');
  Bind(simpleble_peripheral_write_request, 'simpleble_peripheral_write_request');
  Bind(simpleble_peripheral_write_command, 'simpleble_peripheral_write_command');
  Bind(simpleble_peripheral_notify, 'simpleble_peripheral_notify');
  Bind(simpleble_peripheral_indicate, 'simpleble_peripheral_indicate');
  Bind(simpleble_peripheral_unsubscribe, 'simpleble_peripheral_unsubscribe');
  Bind(simpleble_peripheral_read_descriptor, 'simpleble_peripheral_read_descriptor');
  Bind(simpleble_peripheral_write_descriptor, 'simpleble_peripheral_write_descriptor');
  Bind(simpleble_peripheral_set_callback_on_connected, 'simpleble_peripheral_set_callback_on_connected');
  Bind(simpleble_peripheral_set_callback_on_disconnected, 'simpleble_peripheral_set_callback_on_disconnected');
  Bind(simpleble_advanced_dongl_set_passkey_request_callback, 'simpleble_advanced_dongl_set_passkey_request_callback');
  Bind(simpleble_advanced_dongl_set_passkey_display_callback, 'simpleble_advanced_dongl_set_passkey_display_callback');
  Bind(simpleble_advanced_dongl_set_numeric_comparison_callback,
    'simpleble_advanced_dongl_set_numeric_comparison_callback');
  Bind(simpleble_advanced_dongl_secure_connection, 'simpleble_advanced_dongl_secure_connection');
{$IFDEF SIMPLEDELPHIBLE_MACOS}
  Bind(simpleble_advanced_macos_set_advertisement_local_name, 'simpleble_advanced_macos_set_advertisement_local_name');
  Bind(simpleble_advanced_macos_retrieve_cached_peripheral, 'simpleble_advanced_macos_retrieve_cached_peripheral');
{$ENDIF}
{$IFDEF SIMPLEDELPHIBLE_LINUX}
  Bind(simpleble_advanced_linux_set_advertisement_local_name, 'simpleble_advanced_linux_set_advertisement_local_name');
  Bind(simpleble_advanced_linux_set_passkey_request_callback, 'simpleble_advanced_linux_set_passkey_request_callback');
  Bind(simpleble_advanced_linux_set_passkey_display_callback, 'simpleble_advanced_linux_set_passkey_display_callback');
  Bind(simpleble_advanced_linux_set_numeric_comparison_callback,
    'simpleble_advanced_linux_set_numeric_comparison_callback');
  Bind(simpleble_advanced_linux_set_pairing_authorization_callback,
    'simpleble_advanced_linux_set_pairing_authorization_callback');
{$ENDIF}
  Bind(simpleble_backend_get_backends, 'simpleble_backend_get_backends');
  Bind(simpleble_backend_list_get, 'simpleble_backend_list_get');
  Bind(simpleble_backend_list_release, 'simpleble_backend_list_release');
  Bind(simpleble_backend_get_native, 'simpleble_backend_get_native');
  Bind(simpleble_backend_get_dongl, 'simpleble_backend_get_dongl');
  Bind(simpleble_backend_get_simulator, 'simpleble_backend_get_simulator');
  Bind(simpleble_backend_release_handle, 'simpleble_backend_release_handle');
  Bind(simpleble_backend_identifier, 'simpleble_backend_identifier');
  Bind(simpleble_backend_is_bluetooth_enabled, 'simpleble_backend_is_bluetooth_enabled');
  Bind(simpleble_backend_get_adapters, 'simpleble_backend_get_adapters');
  Bind(simpleble_adapter_create_local_peripheral, 'simpleble_adapter_create_local_peripheral');
  Bind(simpleble_local_peripheral_release_handle, 'simpleble_local_peripheral_release_handle');
  Bind(simpleble_local_peripheral_add_advertised_service, 'simpleble_local_peripheral_add_advertised_service');
  Bind(simpleble_local_peripheral_set_advertised_manufacturer_data,
    'simpleble_local_peripheral_set_advertised_manufacturer_data');
  Bind(simpleble_local_peripheral_set_connectable, 'simpleble_local_peripheral_set_connectable');
  Bind(simpleble_local_peripheral_add_service, 'simpleble_local_peripheral_add_service');
  Bind(simpleble_local_peripheral_services, 'simpleble_local_peripheral_services');
  Bind(simpleble_local_peripheral_remove_all_services, 'simpleble_local_peripheral_remove_all_services');
  Bind(simpleble_local_peripheral_start, 'simpleble_local_peripheral_start');
  Bind(simpleble_local_peripheral_stop, 'simpleble_local_peripheral_stop');
  Bind(simpleble_local_peripheral_is_started, 'simpleble_local_peripheral_is_started');
  Bind(simpleble_local_peripheral_is_advertising, 'simpleble_local_peripheral_is_advertising');
  Bind(simpleble_local_peripheral_set_callback_on_client_connected,
    'simpleble_local_peripheral_set_callback_on_client_connected');
  Bind(simpleble_local_peripheral_set_callback_on_client_disconnected,
    'simpleble_local_peripheral_set_callback_on_client_disconnected');
  Bind(simpleble_local_service_release_handle, 'simpleble_local_service_release_handle');
  Bind(simpleble_local_service_identity, 'simpleble_local_service_identity');
  Bind(simpleble_local_service_list_get, 'simpleble_local_service_list_get');
  Bind(simpleble_local_service_list_release, 'simpleble_local_service_list_release');
  Bind(simpleble_local_service_characteristics, 'simpleble_local_service_characteristics');
  Bind(simpleble_local_service_uuid, 'simpleble_local_service_uuid');
  Bind(simpleble_local_service_add_characteristic, 'simpleble_local_service_add_characteristic');
  Bind(simpleble_local_characteristic_release_handle, 'simpleble_local_characteristic_release_handle');
  Bind(simpleble_local_characteristic_identity, 'simpleble_local_characteristic_identity');
  Bind(simpleble_local_characteristic_list_get, 'simpleble_local_characteristic_list_get');
  Bind(simpleble_local_characteristic_list_release, 'simpleble_local_characteristic_list_release');
  Bind(simpleble_local_characteristic_uuid, 'simpleble_local_characteristic_uuid');
  Bind(simpleble_local_characteristic_capabilities, 'simpleble_local_characteristic_capabilities');
  Bind(simpleble_local_characteristic_value, 'simpleble_local_characteristic_value');
  Bind(simpleble_local_characteristic_set_value, 'simpleble_local_characteristic_set_value');
  Bind(simpleble_local_characteristic_set_callback_on_read, 'simpleble_local_characteristic_set_callback_on_read');
  Bind(simpleble_local_characteristic_set_callback_on_write, 'simpleble_local_characteristic_set_callback_on_write');
  Bind(simpleble_local_characteristic_set_callback_on_subscribed,
    'simpleble_local_characteristic_set_callback_on_subscribed');
  Bind(simpleble_local_characteristic_set_callback_on_unsubscribed,
    'simpleble_local_characteristic_set_callback_on_unsubscribed');
  Bind(simpleble_config_reset_all, 'simpleble_config_reset_all');
  Bind(simpleble_config_simplebluez_reset, 'simpleble_config_simplebluez_reset');
  Bind(simpleble_config_simplebluez_get_use_system_bus, 'simpleble_config_simplebluez_get_use_system_bus');
  Bind(simpleble_config_simplebluez_set_use_system_bus, 'simpleble_config_simplebluez_set_use_system_bus');
  Bind(simpleble_config_simplebluez_get_connection_timeout_ms,
    'simpleble_config_simplebluez_get_connection_timeout_ms');
  Bind(simpleble_config_simplebluez_set_connection_timeout_ms,
    'simpleble_config_simplebluez_set_connection_timeout_ms');
  Bind(simpleble_config_simplebluez_get_disconnection_timeout_ms,
    'simpleble_config_simplebluez_get_disconnection_timeout_ms');
  Bind(simpleble_config_simplebluez_set_disconnection_timeout_ms,
    'simpleble_config_simplebluez_set_disconnection_timeout_ms');
  Bind(simpleble_config_simplebluez_get_interactive_pairing, 'simpleble_config_simplebluez_get_interactive_pairing');
  Bind(simpleble_config_simplebluez_set_interactive_pairing, 'simpleble_config_simplebluez_set_interactive_pairing');
  Bind(simpleble_config_winrt_reset, 'simpleble_config_winrt_reset');
  Bind(simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread,
    'simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread');
  Bind(simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread,
    'simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread');
  Bind(simpleble_config_winrt_get_use_deferred_disconnect, 'simpleble_config_winrt_get_use_deferred_disconnect');
  Bind(simpleble_config_winrt_set_use_deferred_disconnect, 'simpleble_config_winrt_set_use_deferred_disconnect');
  Bind(simpleble_config_corebluetooth_reset, 'simpleble_config_corebluetooth_reset');
  Bind(simpleble_config_android_reset, 'simpleble_config_android_reset');
  Bind(simpleble_config_android_get_negotiate_mtu, 'simpleble_config_android_get_negotiate_mtu');
  Bind(simpleble_config_android_set_negotiate_mtu, 'simpleble_config_android_set_negotiate_mtu');
  Bind(simpleble_config_android_get_connection_priority, 'simpleble_config_android_get_connection_priority');
  Bind(simpleble_config_android_set_connection_priority, 'simpleble_config_android_set_connection_priority');
  Bind(simpleble_config_dongl_reset, 'simpleble_config_dongl_reset');
  Bind(simpleble_config_dongl_get_use_dongl_backend, 'simpleble_config_dongl_get_use_dongl_backend');
  Bind(simpleble_config_dongl_set_use_dongl_backend, 'simpleble_config_dongl_set_use_dongl_backend');
  Bind(simpleble_config_dongl_get_auto_update, 'simpleble_config_dongl_get_auto_update');
  Bind(simpleble_config_dongl_set_auto_update, 'simpleble_config_dongl_set_auto_update');
  Bind(simpleble_config_dongl_get_force_update, 'simpleble_config_dongl_get_force_update');
  Bind(simpleble_config_dongl_set_force_update, 'simpleble_config_dongl_set_force_update');
  Bind(simpleble_logging_set_level, 'simpleble_logging_set_level');
  Bind(simpleble_logging_get_level, 'simpleble_logging_get_level');
  Bind(simpleble_logging_set_callback, 'simpleble_logging_set_callback');
  Bind(simpleble_logging_has_callback, 'simpleble_logging_has_callback');
  Bind(simpleble_logging_log_default_stdout, 'simpleble_logging_log_default_stdout');
  Bind(simpleble_logging_log_default_file, 'simpleble_logging_log_default_file');
  Bind(simpleble_logging_log_default_file_path, 'simpleble_logging_log_default_file_path');
  // NOTE: The caller uses the functions only after a successful load, so a failed one leaves nothing in use.
  if Missing <> '' then
  begin
    FreeLibrary(Lib);
    Error := FileName + ' has no ' + Missing + ', so it isn''t the SimpleBLE version this binding needs';
    Exit(False);
  end;
  Error := '';
  Result := True;
end;

end.
