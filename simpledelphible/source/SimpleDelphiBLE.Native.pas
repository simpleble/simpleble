// Declarations of the SimpleCBLE C API (simplecble/include/simplecble), as the C headers define them.
unit SimpleDelphiBLE.Native;

{$I SimpleDelphiBLE.inc}

{$IFDEF FPC}
  {$PACKRECORDS C}
  {$IFDEF UNIX}
    // NOTE: Free Pascal on macOS doesn't link the library its external declarations name.
    {$LINKLIB simplecble}
  {$ENDIF}
{$ENDIF}

interface

const
{$IF DEFINED(MSWINDOWS)}
  SimpleCBLELib = 'simplecble.dll';
{$ELSEIF DEFINED(FPC)}
  SimpleCBLELib = 'simplecble';
{$ELSEIF DEFINED(MACOS)}
  SimpleCBLELib = 'libsimplecble.dylib';
{$ELSE}
  SimpleCBLELib = 'libsimplecble.so';
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

// simplecble.h
procedure simpleble_free(handle: Pointer); cdecl; external SimpleCBLELib;

// error.h
function simpleble_error_code(error: psimpleble_error_t): simpleble_err_t; cdecl; external SimpleCBLELib;
function simpleble_error_message(error: psimpleble_error_t): PAnsiChar; cdecl; external SimpleCBLELib;
procedure simpleble_error_release(var error: psimpleble_error_t); cdecl; external SimpleCBLELib;

// utils.h
function simpleble_get_operating_system: simpleble_os_t; cdecl; external SimpleCBLELib;
function simpleble_get_version: PAnsiChar; cdecl; external SimpleCBLELib;

// adapter.h
function simpleble_adapter_is_bluetooth_enabled(var out_error: psimpleble_error_t): Boolean; cdecl;
  external SimpleCBLELib;
function simpleble_adapter_get_adapters(out out_list: simpleble_adapter_list_t;
  var out_error: psimpleble_error_t): NativeUInt; cdecl; external SimpleCBLELib;
function simpleble_adapter_list_get(list: simpleble_adapter_list_t; index: NativeUInt;
  var out_error: psimpleble_error_t): simpleble_adapter_t; cdecl; external SimpleCBLELib;
procedure simpleble_adapter_list_release(list: simpleble_adapter_list_t); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_release_handle(handle: simpleble_adapter_t); cdecl; external SimpleCBLELib;
function simpleble_adapter_identity(handle: simpleble_adapter_t): Pointer; cdecl; external SimpleCBLELib;
function simpleble_adapter_identifier(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): PAnsiChar;
  cdecl; external SimpleCBLELib;
function simpleble_adapter_address(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): PAnsiChar;
  cdecl; external SimpleCBLELib;
procedure simpleble_adapter_power_on(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_adapter_power_off(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
function simpleble_adapter_is_powered(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): Boolean;
  cdecl; external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_power_on(handle: simpleble_adapter_t;
  callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_power_off(handle: simpleble_adapter_t;
  callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_scan_start(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_adapter_scan_stop(handle: simpleble_adapter_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
function simpleble_adapter_scan_is_active(handle: simpleble_adapter_t; var out_error: psimpleble_error_t): Boolean;
  cdecl; external SimpleCBLELib;
procedure simpleble_adapter_scan_for(handle: simpleble_adapter_t; timeout_ms: Int32;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_adapter_scan_get_results(handle: simpleble_adapter_t; out out_list: simpleble_peripheral_list_t;
  var out_error: psimpleble_error_t): NativeUInt; cdecl; external SimpleCBLELib;
function simpleble_adapter_get_paired_peripherals(handle: simpleble_adapter_t;
  out out_list: simpleble_peripheral_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  external SimpleCBLELib;
function simpleble_adapter_get_connected_peripherals(handle: simpleble_adapter_t;
  out out_list: simpleble_peripheral_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_scan_start(handle: simpleble_adapter_t;
  callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_scan_stop(handle: simpleble_adapter_t;
  callback: simpleble_adapter_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_scan_updated(handle: simpleble_adapter_t;
  callback: simpleble_adapter_peripheral_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_adapter_set_callback_on_scan_found(handle: simpleble_adapter_t;
  callback: simpleble_adapter_peripheral_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;

// peripheral.h
procedure simpleble_peripheral_release_handle(handle: simpleble_peripheral_t); cdecl; external SimpleCBLELib;
function simpleble_peripheral_identity(handle: simpleble_peripheral_t): Pointer; cdecl; external SimpleCBLELib;
function simpleble_peripheral_list_get(list: simpleble_peripheral_list_t; index: NativeUInt;
  var out_error: psimpleble_error_t): simpleble_peripheral_t; cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_list_release(list: simpleble_peripheral_list_t); cdecl; external SimpleCBLELib;
function simpleble_peripheral_identifier(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): PAnsiChar;
  cdecl; external SimpleCBLELib;
function simpleble_peripheral_address(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): PAnsiChar;
  cdecl; external SimpleCBLELib;
function simpleble_peripheral_address_type(handle: simpleble_peripheral_t;
  var out_error: psimpleble_error_t): simpleble_address_type_t; cdecl; external SimpleCBLELib;
function simpleble_peripheral_rssi(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Int16; cdecl;
  external SimpleCBLELib;
function simpleble_peripheral_tx_power(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Int16;
  cdecl; external SimpleCBLELib;
function simpleble_peripheral_mtu(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): UInt16; cdecl;
  external SimpleCBLELib;
function simpleble_peripheral_is_connected(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Boolean;
  cdecl; external SimpleCBLELib;
function simpleble_peripheral_is_connectable(handle: simpleble_peripheral_t;
  var out_error: psimpleble_error_t): Boolean; cdecl; external SimpleCBLELib;
function simpleble_peripheral_manufacturer_data(handle: simpleble_peripheral_t;
  out out_list: simpleble_manufacturer_data_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  external SimpleCBLELib;
procedure simpleble_manufacturer_data_list_get(list: simpleble_manufacturer_data_list_t; index: NativeUInt;
  var out_data: simpleble_manufacturer_data_t; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_manufacturer_data_list_release(list: simpleble_manufacturer_data_list_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_manufacturer_data_release(var data: simpleble_manufacturer_data_t); cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_connect(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_peripheral_disconnect(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
function simpleble_peripheral_is_paired(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t): Boolean;
  cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_unpair(handle: simpleble_peripheral_t; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
function simpleble_peripheral_services(handle: simpleble_peripheral_t; out out_list: simpleble_service_list_t;
  var out_error: psimpleble_error_t): NativeUInt; cdecl; external SimpleCBLELib;
procedure simpleble_service_list_get(list: simpleble_service_list_t; index: NativeUInt;
  var out_service: simpleble_service_t; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_service_list_release(list: simpleble_service_list_t); cdecl; external SimpleCBLELib;
procedure simpleble_service_release(var service: simpleble_service_t); cdecl; external SimpleCBLELib;
function simpleble_peripheral_read(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; out data_length: NativeUInt; var out_error: psimpleble_error_t): PByte; cdecl;
  external SimpleCBLELib;
procedure simpleble_peripheral_write_request(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_peripheral_write_command(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_peripheral_notify(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; callback: simpleble_peripheral_value_callback_t; userdata: Pointer;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_indicate(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; callback: simpleble_peripheral_value_callback_t; userdata: Pointer;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_unsubscribe(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_peripheral_read_descriptor(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; descriptor: simpleble_uuid_t; out data_length: NativeUInt;
  var out_error: psimpleble_error_t): PByte; cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_write_descriptor(handle: simpleble_peripheral_t; service: simpleble_uuid_t;
  characteristic: simpleble_uuid_t; descriptor: simpleble_uuid_t; data: PByte; data_length: NativeUInt;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_set_callback_on_connected(handle: simpleble_peripheral_t;
  callback: simpleble_peripheral_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_peripheral_set_callback_on_disconnected(handle: simpleble_peripheral_t;
  callback: simpleble_peripheral_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;

// advanced.h, the parts every build has
procedure simpleble_advanced_dongl_set_passkey_request_callback(handle: simpleble_peripheral_t;
  callback: simpleble_dongl_passkey_request_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_advanced_dongl_set_passkey_display_callback(handle: simpleble_peripheral_t;
  callback: simpleble_dongl_passkey_display_callback_t; userdata: Pointer; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_advanced_dongl_set_numeric_comparison_callback(handle: simpleble_peripheral_t;
  callback: simpleble_dongl_numeric_comparison_callback_t; userdata: Pointer; var out_error: psimpleble_error_t);
  cdecl; external SimpleCBLELib;
procedure simpleble_advanced_dongl_secure_connection(handle: simpleble_peripheral_t; mitm: Boolean;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;

{$IFDEF SIMPLEDELPHIBLE_MACOS}
// advanced.h, macOS builds
procedure simpleble_advanced_macos_set_advertisement_local_name(handle: simpleble_local_peripheral_t;
  local_name: PAnsiChar; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_advanced_macos_retrieve_cached_peripheral(handle: simpleble_adapter_t; identifier: PAnsiChar;
  var out_error: psimpleble_error_t): simpleble_peripheral_t; cdecl; external SimpleCBLELib;
{$ENDIF}

{$IFDEF SIMPLEDELPHIBLE_LINUX}
// advanced.h, Linux builds
procedure simpleble_advanced_linux_set_advertisement_local_name(handle: simpleble_local_peripheral_t;
  local_name: PAnsiChar; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
{$ENDIF}

// backend.h
function simpleble_backend_get_backends(out out_list: simpleble_backend_list_t;
  var out_error: psimpleble_error_t): NativeUInt; cdecl; external SimpleCBLELib;
function simpleble_backend_list_get(list: simpleble_backend_list_t; index: NativeUInt;
  var out_error: psimpleble_error_t): simpleble_backend_t; cdecl; external SimpleCBLELib;
procedure simpleble_backend_list_release(list: simpleble_backend_list_t); cdecl; external SimpleCBLELib;
function simpleble_backend_get_native(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  external SimpleCBLELib;
function simpleble_backend_get_dongl(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  external SimpleCBLELib;
function simpleble_backend_get_simulator(var out_error: psimpleble_error_t): simpleble_backend_t; cdecl;
  external SimpleCBLELib;
procedure simpleble_backend_release_handle(handle: simpleble_backend_t); cdecl; external SimpleCBLELib;
function simpleble_backend_identifier(handle: simpleble_backend_t; var out_error: psimpleble_error_t): PAnsiChar;
  cdecl; external SimpleCBLELib;
function simpleble_backend_is_bluetooth_enabled(handle: simpleble_backend_t;
  var out_error: psimpleble_error_t): Boolean; cdecl; external SimpleCBLELib;
function simpleble_backend_get_adapters(handle: simpleble_backend_t; out out_list: simpleble_adapter_list_t;
  var out_error: psimpleble_error_t): NativeUInt; cdecl; external SimpleCBLELib;

// adapter.h, local peripherals
function simpleble_adapter_create_local_peripheral(handle: simpleble_adapter_t;
  var out_error: psimpleble_error_t): simpleble_local_peripheral_t; cdecl; external SimpleCBLELib;

// local/peripheral.h
procedure simpleble_local_peripheral_release_handle(handle: simpleble_local_peripheral_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_local_peripheral_add_advertised_service(handle: simpleble_local_peripheral_t;
  service: simpleble_uuid_t; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_local_peripheral_set_advertised_manufacturer_data(handle: simpleble_local_peripheral_t;
  company_id: UInt16; data: PByte; data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_local_peripheral_set_connectable(handle: simpleble_local_peripheral_t; connectable: Boolean;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_local_peripheral_add_service(handle: simpleble_local_peripheral_t; uuid: simpleble_uuid_t;
  var out_error: psimpleble_error_t): simpleble_local_service_t; cdecl; external SimpleCBLELib;
function simpleble_local_peripheral_services(handle: simpleble_local_peripheral_t;
  out out_list: simpleble_local_service_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  external SimpleCBLELib;
procedure simpleble_local_peripheral_remove_all_services(handle: simpleble_local_peripheral_t;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_local_peripheral_start(handle: simpleble_local_peripheral_t; var out_error: psimpleble_error_t);
  cdecl; external SimpleCBLELib;
procedure simpleble_local_peripheral_stop(handle: simpleble_local_peripheral_t; var out_error: psimpleble_error_t);
  cdecl; external SimpleCBLELib;
function simpleble_local_peripheral_is_started(handle: simpleble_local_peripheral_t;
  var out_error: psimpleble_error_t): Boolean; cdecl; external SimpleCBLELib;
function simpleble_local_peripheral_is_advertising(handle: simpleble_local_peripheral_t;
  var out_error: psimpleble_error_t): Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_local_peripheral_set_callback_on_client_connected(handle: simpleble_local_peripheral_t;
  callback: simpleble_local_peripheral_client_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_local_peripheral_set_callback_on_client_disconnected(handle: simpleble_local_peripheral_t;
  callback: simpleble_local_peripheral_client_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;

// local/service.h
procedure simpleble_local_service_release_handle(handle: simpleble_local_service_t); cdecl; external SimpleCBLELib;
function simpleble_local_service_identity(handle: simpleble_local_service_t): Pointer; cdecl; external SimpleCBLELib;
function simpleble_local_service_list_get(list: simpleble_local_service_list_t; index: NativeUInt;
  var out_error: psimpleble_error_t): simpleble_local_service_t; cdecl; external SimpleCBLELib;
procedure simpleble_local_service_list_release(list: simpleble_local_service_list_t); cdecl; external SimpleCBLELib;
function simpleble_local_service_characteristics(handle: simpleble_local_service_t;
  out out_list: simpleble_local_characteristic_list_t; var out_error: psimpleble_error_t): NativeUInt; cdecl;
  external SimpleCBLELib;
procedure simpleble_local_service_uuid(handle: simpleble_local_service_t; out out_uuid: simpleble_uuid_t;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_local_service_add_characteristic(handle: simpleble_local_service_t; uuid: simpleble_uuid_t;
  capabilities: UInt32; var out_error: psimpleble_error_t): simpleble_local_characteristic_t; cdecl;
  external SimpleCBLELib;

// local/characteristic.h
procedure simpleble_local_characteristic_release_handle(handle: simpleble_local_characteristic_t); cdecl;
  external SimpleCBLELib;
function simpleble_local_characteristic_identity(handle: simpleble_local_characteristic_t): Pointer; cdecl;
  external SimpleCBLELib;
function simpleble_local_characteristic_list_get(list: simpleble_local_characteristic_list_t; index: NativeUInt;
  var out_error: psimpleble_error_t): simpleble_local_characteristic_t; cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_list_release(list: simpleble_local_characteristic_list_t); cdecl;
  external SimpleCBLELib;
procedure simpleble_local_characteristic_uuid(handle: simpleble_local_characteristic_t; out out_uuid: simpleble_uuid_t;
  var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
function simpleble_local_characteristic_capabilities(handle: simpleble_local_characteristic_t;
  var out_error: psimpleble_error_t): UInt32; cdecl; external SimpleCBLELib;
function simpleble_local_characteristic_value(handle: simpleble_local_characteristic_t; out data_length: NativeUInt;
  var out_error: psimpleble_error_t): PByte; cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_set_value(handle: simpleble_local_characteristic_t; data: PByte;
  data_length: NativeUInt; var out_error: psimpleble_error_t); cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_set_callback_on_read(handle: simpleble_local_characteristic_t;
  callback: simpleble_local_characteristic_read_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_set_callback_on_write(handle: simpleble_local_characteristic_t;
  callback: simpleble_local_characteristic_write_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_set_callback_on_subscribed(handle: simpleble_local_characteristic_t;
  callback: simpleble_local_characteristic_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;
procedure simpleble_local_characteristic_set_callback_on_unsubscribed(handle: simpleble_local_characteristic_t;
  callback: simpleble_local_characteristic_callback_t; userdata: Pointer); cdecl; external SimpleCBLELib;

// config.h
procedure simpleble_config_reset_all; cdecl; external SimpleCBLELib;
procedure simpleble_config_simplebluez_reset; cdecl; external SimpleCBLELib;
function simpleble_config_simplebluez_get_use_system_bus: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_simplebluez_set_use_system_bus(enabled: Boolean); cdecl; external SimpleCBLELib;
function simpleble_config_simplebluez_get_connection_timeout_ms: Int64; cdecl; external SimpleCBLELib;
procedure simpleble_config_simplebluez_set_connection_timeout_ms(timeout_ms: Int64); cdecl; external SimpleCBLELib;
function simpleble_config_simplebluez_get_disconnection_timeout_ms: Int64; cdecl; external SimpleCBLELib;
procedure simpleble_config_simplebluez_set_disconnection_timeout_ms(timeout_ms: Int64); cdecl; external SimpleCBLELib;
procedure simpleble_config_winrt_reset; cdecl; external SimpleCBLELib;
function simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread: Boolean; cdecl;
  external SimpleCBLELib;
procedure simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(enabled: Boolean);
  cdecl; external SimpleCBLELib;
function simpleble_config_winrt_get_use_deferred_disconnect: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_winrt_set_use_deferred_disconnect(enabled: Boolean); cdecl; external SimpleCBLELib;
procedure simpleble_config_corebluetooth_reset; cdecl; external SimpleCBLELib;
procedure simpleble_config_android_reset; cdecl; external SimpleCBLELib;
function simpleble_config_android_get_negotiate_mtu: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_android_set_negotiate_mtu(enabled: Boolean); cdecl; external SimpleCBLELib;
function simpleble_config_android_get_connection_priority: simpleble_config_android_connection_priority_t; cdecl;
  external SimpleCBLELib;
procedure simpleble_config_android_set_connection_priority(priority: simpleble_config_android_connection_priority_t);
  cdecl; external SimpleCBLELib;
procedure simpleble_config_dongl_reset; cdecl; external SimpleCBLELib;
function simpleble_config_dongl_get_use_dongl_backend: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_dongl_set_use_dongl_backend(enabled: Boolean); cdecl; external SimpleCBLELib;
function simpleble_config_dongl_get_auto_update: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_dongl_set_auto_update(enabled: Boolean); cdecl; external SimpleCBLELib;
function simpleble_config_dongl_get_force_update: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_config_dongl_set_force_update(enabled: Boolean); cdecl; external SimpleCBLELib;

// logging.h
procedure simpleble_logging_set_level(level: simpleble_log_level_t); cdecl; external SimpleCBLELib;
function simpleble_logging_get_level: simpleble_log_level_t; cdecl; external SimpleCBLELib;
procedure simpleble_logging_set_callback(callback: simpleble_log_callback_t); cdecl; external SimpleCBLELib;
function simpleble_logging_has_callback: Boolean; cdecl; external SimpleCBLELib;
procedure simpleble_logging_log_default_stdout; cdecl; external SimpleCBLELib;
procedure simpleble_logging_log_default_file; cdecl; external SimpleCBLELib;
procedure simpleble_logging_log_default_file_path(path: PAnsiChar); cdecl; external SimpleCBLELib;

implementation

end.
