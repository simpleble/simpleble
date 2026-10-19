package bindings

import (
	"fmt"
	"runtime"
	"sync"
	"unsafe"

	"github.com/ebitengine/purego"
)

// Keep C pointers opaque, use fixed-width C int/enum types, and pass actual Go
// pointers for synchronous out parameters so purego keeps them alive.
type NativeAPI struct {
	Free                              func(unsafe.Pointer)
	Version                           func() string
	ErrorCode                         func(uintptr) uint32
	ErrorMessage                      func(uintptr) string
	ErrorRelease                      func(*uintptr)
	Backends                          func(*uintptr, *uintptr) uintptr
	BackendListGet                    func(uintptr, uintptr, *uintptr) uintptr
	BackendListRelease                func(uintptr)
	BackendNative                     func(*uintptr) uintptr
	BackendDongl                      func(*uintptr) uintptr
	BackendSimulator                  func(*uintptr) uintptr
	BackendRelease                    func(uintptr)
	BackendIdentifier                 func(uintptr, *uintptr) *byte
	BackendEnabled                    func(uintptr, *uintptr) bool
	BackendAdapters                   func(uintptr, *uintptr, *uintptr) uintptr
	Adapters                          func(*uintptr, *uintptr) uintptr
	AdapterListGet                    func(uintptr, uintptr, *uintptr) uintptr
	AdapterListRelease                func(uintptr)
	AdapterRelease                    func(uintptr)
	AdapterEnabled                    func(*uintptr) bool
	AdapterIdentifier                 func(uintptr, *uintptr) *byte
	AdapterAddress                    func(uintptr, *uintptr) *byte
	AdapterPowerOn                    func(uintptr, *uintptr)
	AdapterPowerOff                   func(uintptr, *uintptr)
	AdapterPowered                    func(uintptr, *uintptr) bool
	AdapterScanStart                  func(uintptr, *uintptr)
	AdapterScanStop                   func(uintptr, *uintptr)
	AdapterScanFor                    func(uintptr, int32, *uintptr)
	AdapterScanActive                 func(uintptr, *uintptr) bool
	SetCallbacks                      [4]func(uintptr, uintptr, uintptr)
	AdapterScanResults                func(uintptr, *uintptr, *uintptr) uintptr
	AdapterPaired                     func(uintptr, *uintptr, *uintptr) uintptr
	AdapterConnected                  func(uintptr, *uintptr, *uintptr) uintptr
	PeripheralListGet                 func(uintptr, uintptr, *uintptr) uintptr
	PeripheralListRelease             func(uintptr)
	AdapterScanFound                  func(uintptr, uintptr, uintptr)
	AdapterScanUpdated                func(uintptr, uintptr, uintptr)
	PeripheralRelease                 func(uintptr)
	PeripheralIdentifier              func(uintptr, *uintptr) *byte
	PeripheralAddress                 func(uintptr, *uintptr) *byte
	PeripheralAddressType             func(uintptr, *uintptr) uint32
	PeripheralRSSI                    func(uintptr, *uintptr) int16
	PeripheralTxPower                 func(uintptr, *uintptr) int16
	PeripheralMTU                     func(uintptr, *uintptr) uint16
	PeripheralConnect                 func(uintptr, *uintptr)
	PeripheralDisconnect              func(uintptr, *uintptr)
	PeripheralConnected               func(uintptr, *uintptr) bool
	PeripheralConnectable             func(uintptr, *uintptr) bool
	PeripheralPaired                  func(uintptr, *uintptr) bool
	PeripheralUnpair                  func(uintptr, *uintptr)
	PeripheralServices                func(uintptr, *uintptr, *uintptr) uintptr
	ServiceListGet                    func(uintptr, uintptr, *Service, *uintptr)
	ServiceListRelease                func(uintptr)
	ServiceRelease                    func(*Service)
	PeripheralManufacturerData        func(uintptr, *uintptr, *uintptr) uintptr
	ManufacturerListGet               func(uintptr, uintptr, *ManufacturerData, *uintptr)
	ManufacturerListRelease           func(uintptr)
	ManufacturerRelease               func(*ManufacturerData)
	PeripheralRead                    func(uintptr, UUID, UUID, *uintptr, *uintptr) *byte
	PeripheralReadDescriptor          func(uintptr, UUID, UUID, UUID, *uintptr, *uintptr) *byte
	PeripheralWriteRequest            func(uintptr, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralWriteCommand            func(uintptr, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralWriteDescriptor         func(uintptr, UUID, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralNotify                  func(uintptr, UUID, UUID, uintptr, uintptr, *uintptr)
	PeripheralIndicate                func(uintptr, UUID, UUID, uintptr, uintptr, *uintptr)
	PeripheralUnsubscribe             func(uintptr, UUID, UUID, *uintptr)
	PeripheralOnConnected             func(uintptr, uintptr, uintptr)
	PeripheralOnDisconnected          func(uintptr, uintptr, uintptr)
	AdapterCreateLocalPeripheral      func(uintptr, *uintptr) uintptr
	LocalPeripheralRelease            func(uintptr)
	LocalPeripheralAdvertiseService   func(uintptr, UUID, *uintptr)
	LocalPeripheralManufacturerData   func(uintptr, uint16, *byte, uintptr, *uintptr)
	LocalPeripheralSetConnectable     func(uintptr, bool, *uintptr)
	LocalPeripheralAddService         func(uintptr, UUID, *uintptr) uintptr
	LocalPeripheralServices           func(uintptr, *uintptr, *uintptr) uintptr
	LocalPeripheralRemoveServices     func(uintptr, *uintptr)
	LocalPeripheralStart              func(uintptr, *uintptr)
	LocalPeripheralStop               func(uintptr, *uintptr)
	LocalPeripheralStarted            func(uintptr, *uintptr) bool
	LocalPeripheralAdvertising        func(uintptr, *uintptr) bool
	LocalPeripheralOnConnected        func(uintptr, uintptr, uintptr)
	LocalPeripheralOnDisconnected     func(uintptr, uintptr, uintptr)
	LocalServiceRelease               func(uintptr)
	LocalServiceIdentity              func(uintptr) uintptr
	LocalServiceListGet               func(uintptr, uintptr, *uintptr) uintptr
	LocalServiceListRelease           func(uintptr)
	LocalServiceUUID                  func(uintptr, *UUID, *uintptr)
	LocalServiceAddCharacteristic     func(uintptr, UUID, uint32, *uintptr) uintptr
	LocalServiceCharacteristics       func(uintptr, *uintptr, *uintptr) uintptr
	LocalCharacteristicRelease        func(uintptr)
	LocalCharacteristicIdentity       func(uintptr) uintptr
	LocalCharacteristicListGet        func(uintptr, uintptr, *uintptr) uintptr
	LocalCharacteristicListRelease    func(uintptr)
	LocalCharacteristicUUID           func(uintptr, *UUID, *uintptr)
	LocalCharacteristicCapabilities   func(uintptr, *uintptr) uint32
	LocalCharacteristicValue          func(uintptr, *uintptr, *uintptr) *byte
	LocalCharacteristicSetValue       func(uintptr, *byte, uintptr, *uintptr)
	LocalCharacteristicOnRead         func(uintptr, uintptr, uintptr)
	LocalCharacteristicOnWrite        func(uintptr, uintptr, uintptr)
	LocalCharacteristicOnSubscribed   func(uintptr, uintptr, uintptr)
	LocalCharacteristicOnUnsubscribed func(uintptr, uintptr, uintptr)
	ConfigResetAll                    func()
	ConfigBluezReset                  func()
	ConfigBluezSystemBus              func() bool
	ConfigBluezSetSystemBus           func(bool)
	ConfigBluezConnectTimeout         func() int64
	ConfigBluezSetConnectTimeout      func(int64)
	ConfigBluezDisconnectTimeout      func() int64
	ConfigBluezSetDisconnectTimeout   func(int64)
	ConfigWinRTReset                  func()
	ConfigWinRTOwnMTA                 func() bool
	ConfigWinRTSetOwnMTA              func(bool)
	ConfigWinRTReinitialize           func() bool
	ConfigWinRTSetReinitialize        func(bool)
	ConfigWinRTDeferredDisconnect     func() bool
	ConfigWinRTSetDeferredDisconnect  func(bool)
	ConfigCoreBluetoothReset          func()
	ConfigDonglReset                  func()
	ConfigDonglBackend                func() bool
	ConfigDonglSetBackend             func(bool)
	ConfigDonglAutoUpdate             func() bool
	ConfigDonglSetAutoUpdate          func(bool)
	ConfigDonglForceUpdate            func() bool
	ConfigDonglSetForceUpdate         func(bool)
	DonglPasskeyRequest               func(uintptr, uintptr, uintptr, *uintptr)
	DonglPasskeyDisplay               func(uintptr, uintptr, uintptr, *uintptr)
	DonglNumericComparison            func(uintptr, uintptr, uintptr, *uintptr)
	SetAdvertisementLocalName         func(uintptr, *byte, *uintptr) // Optional; see Load.
	RetrieveCachedPeripheral          func(uintptr, *byte, *uintptr) uintptr
}

var loadedLibrary struct {
	sync.Mutex
	api  *NativeAPI
	path string
}

// Load loads SimpleCBLE once per process. An empty path selects the packaged
// native library, extracting it into the user's cache; native tests pass the
// path of a PLAIN build. A different library cannot replace a loaded one.
// Failed loads can be retried.
//
// Enumeration and BluetoothEnabled call Load("") automatically if needed.
func Load(path string) error {
	loadedLibrary.Lock()
	defer loadedLibrary.Unlock()
	if loadedLibrary.api != nil {
		if path != "" && path != loadedLibrary.path {
			return fmt.Errorf("simplegoble: already loaded %q", loadedLibrary.path)
		}
		return nil
	}
	if path == "" {
		if bundledLibraries == nil {
			return fmt.Errorf("simplegoble: native library bundle is missing")
		}
		var err error
		path, err = bundledLibraryPath()
		if err != nil {
			return fmt.Errorf("simplegoble: extract bundled library: %w", err)
		}
	}
	libraryHandle, err := openLibrary(path)
	if err != nil {
		return fmt.Errorf("simplegoble: load %q: %w", path, err)
	}
	native := new(NativeAPI)
	bindings := []struct {
		name   string
		target any
	}{
		{"simpleble_free", &native.Free},
		{"simpleble_get_version", &native.Version},
		{"simpleble_error_code", &native.ErrorCode},
		{"simpleble_error_message", &native.ErrorMessage},
		{"simpleble_error_release", &native.ErrorRelease},
		{"simpleble_backend_get_backends", &native.Backends},
		{"simpleble_backend_list_get", &native.BackendListGet},
		{"simpleble_backend_list_release", &native.BackendListRelease},
		{"simpleble_backend_get_native", &native.BackendNative},
		{"simpleble_backend_get_dongl", &native.BackendDongl},
		{"simpleble_backend_get_simulator", &native.BackendSimulator},
		{"simpleble_backend_release_handle", &native.BackendRelease},
		{"simpleble_backend_identifier", &native.BackendIdentifier},
		{"simpleble_backend_is_bluetooth_enabled", &native.BackendEnabled},
		{"simpleble_backend_get_adapters", &native.BackendAdapters},
		{"simpleble_adapter_get_adapters", &native.Adapters},
		{"simpleble_adapter_list_get", &native.AdapterListGet},
		{"simpleble_adapter_list_release", &native.AdapterListRelease},
		{"simpleble_adapter_release_handle", &native.AdapterRelease},
		{"simpleble_adapter_is_bluetooth_enabled", &native.AdapterEnabled},
		{"simpleble_adapter_identifier", &native.AdapterIdentifier},
		{"simpleble_adapter_address", &native.AdapterAddress},
		{"simpleble_adapter_power_on", &native.AdapterPowerOn},
		{"simpleble_adapter_power_off", &native.AdapterPowerOff},
		{"simpleble_adapter_is_powered", &native.AdapterPowered},
		{"simpleble_adapter_scan_start", &native.AdapterScanStart},
		{"simpleble_adapter_scan_stop", &native.AdapterScanStop},
		{"simpleble_adapter_scan_for", &native.AdapterScanFor},
		{"simpleble_adapter_scan_is_active", &native.AdapterScanActive},
		{"simpleble_adapter_set_callback_on_power_on", &native.SetCallbacks[PowerOn]},
		{"simpleble_adapter_set_callback_on_power_off", &native.SetCallbacks[PowerOff]},
		{"simpleble_adapter_set_callback_on_scan_start", &native.SetCallbacks[ScanStart]},
		{"simpleble_adapter_set_callback_on_scan_stop", &native.SetCallbacks[ScanStop]},
		{"simpleble_adapter_scan_get_results", &native.AdapterScanResults},
		{"simpleble_adapter_get_paired_peripherals", &native.AdapterPaired},
		{"simpleble_adapter_get_connected_peripherals", &native.AdapterConnected},
		{"simpleble_peripheral_list_get", &native.PeripheralListGet},
		{"simpleble_peripheral_list_release", &native.PeripheralListRelease},
		{"simpleble_adapter_set_callback_on_scan_found", &native.AdapterScanFound},
		{"simpleble_adapter_set_callback_on_scan_updated", &native.AdapterScanUpdated},
		{"simpleble_peripheral_release_handle", &native.PeripheralRelease},
		{"simpleble_peripheral_identifier", &native.PeripheralIdentifier},
		{"simpleble_peripheral_address", &native.PeripheralAddress},
		{"simpleble_peripheral_address_type", &native.PeripheralAddressType},
		{"simpleble_peripheral_rssi", &native.PeripheralRSSI},
		{"simpleble_peripheral_tx_power", &native.PeripheralTxPower},
		{"simpleble_peripheral_mtu", &native.PeripheralMTU},
		{"simpleble_peripheral_connect", &native.PeripheralConnect},
		{"simpleble_peripheral_disconnect", &native.PeripheralDisconnect},
		{"simpleble_peripheral_is_connected", &native.PeripheralConnected},
		{"simpleble_peripheral_is_connectable", &native.PeripheralConnectable},
		{"simpleble_peripheral_is_paired", &native.PeripheralPaired},
		{"simpleble_peripheral_unpair", &native.PeripheralUnpair},
		{"simpleble_peripheral_services", &native.PeripheralServices},
		{"simpleble_service_list_get", &native.ServiceListGet},
		{"simpleble_service_list_release", &native.ServiceListRelease},
		{"simpleble_service_release", &native.ServiceRelease},
		{"simpleble_peripheral_manufacturer_data", &native.PeripheralManufacturerData},
		{"simpleble_manufacturer_data_list_get", &native.ManufacturerListGet},
		{"simpleble_manufacturer_data_list_release", &native.ManufacturerListRelease},
		{"simpleble_manufacturer_data_release", &native.ManufacturerRelease},
		{"simpleble_peripheral_read", &native.PeripheralRead},
		{"simpleble_peripheral_read_descriptor", &native.PeripheralReadDescriptor},
		{"simpleble_peripheral_write_request", &native.PeripheralWriteRequest},
		{"simpleble_peripheral_write_command", &native.PeripheralWriteCommand},
		{"simpleble_peripheral_write_descriptor", &native.PeripheralWriteDescriptor},
		{"simpleble_peripheral_notify", &native.PeripheralNotify},
		{"simpleble_peripheral_indicate", &native.PeripheralIndicate},
		{"simpleble_peripheral_unsubscribe", &native.PeripheralUnsubscribe},
		{"simpleble_peripheral_set_callback_on_connected", &native.PeripheralOnConnected},
		{"simpleble_peripheral_set_callback_on_disconnected", &native.PeripheralOnDisconnected},
		{"simpleble_adapter_create_local_peripheral", &native.AdapterCreateLocalPeripheral},
		{"simpleble_local_peripheral_release_handle", &native.LocalPeripheralRelease},
		{"simpleble_local_peripheral_add_advertised_service", &native.LocalPeripheralAdvertiseService},
		{"simpleble_local_peripheral_set_advertised_manufacturer_data", &native.LocalPeripheralManufacturerData},
		{"simpleble_local_peripheral_set_connectable", &native.LocalPeripheralSetConnectable},
		{"simpleble_local_peripheral_add_service", &native.LocalPeripheralAddService},
		{"simpleble_local_peripheral_services", &native.LocalPeripheralServices},
		{"simpleble_local_peripheral_remove_all_services", &native.LocalPeripheralRemoveServices},
		{"simpleble_local_peripheral_start", &native.LocalPeripheralStart},
		{"simpleble_local_peripheral_stop", &native.LocalPeripheralStop},
		{"simpleble_local_peripheral_is_started", &native.LocalPeripheralStarted},
		{"simpleble_local_peripheral_is_advertising", &native.LocalPeripheralAdvertising},
		{"simpleble_local_peripheral_set_callback_on_client_connected", &native.LocalPeripheralOnConnected},
		{"simpleble_local_peripheral_set_callback_on_client_disconnected", &native.LocalPeripheralOnDisconnected},
		{"simpleble_local_service_release_handle", &native.LocalServiceRelease},
		{"simpleble_local_service_identity", &native.LocalServiceIdentity},
		{"simpleble_local_service_list_get", &native.LocalServiceListGet},
		{"simpleble_local_service_list_release", &native.LocalServiceListRelease},
		{"simpleble_local_service_uuid", &native.LocalServiceUUID},
		{"simpleble_local_service_add_characteristic", &native.LocalServiceAddCharacteristic},
		{"simpleble_local_service_characteristics", &native.LocalServiceCharacteristics},
		{"simpleble_local_characteristic_release_handle", &native.LocalCharacteristicRelease},
		{"simpleble_local_characteristic_identity", &native.LocalCharacteristicIdentity},
		{"simpleble_local_characteristic_list_get", &native.LocalCharacteristicListGet},
		{"simpleble_local_characteristic_list_release", &native.LocalCharacteristicListRelease},
		{"simpleble_local_characteristic_uuid", &native.LocalCharacteristicUUID},
		{"simpleble_local_characteristic_capabilities", &native.LocalCharacteristicCapabilities},
		{"simpleble_local_characteristic_value", &native.LocalCharacteristicValue},
		{"simpleble_local_characteristic_set_value", &native.LocalCharacteristicSetValue},
		{"simpleble_local_characteristic_set_callback_on_read", &native.LocalCharacteristicOnRead},
		{"simpleble_local_characteristic_set_callback_on_write", &native.LocalCharacteristicOnWrite},
		{"simpleble_local_characteristic_set_callback_on_subscribed", &native.LocalCharacteristicOnSubscribed},
		{"simpleble_local_characteristic_set_callback_on_unsubscribed", &native.LocalCharacteristicOnUnsubscribed},
		{"simpleble_config_reset_all", &native.ConfigResetAll},
		{"simpleble_config_simplebluez_reset", &native.ConfigBluezReset},
		{"simpleble_config_simplebluez_get_use_system_bus", &native.ConfigBluezSystemBus},
		{"simpleble_config_simplebluez_set_use_system_bus", &native.ConfigBluezSetSystemBus},
		{"simpleble_config_simplebluez_get_connection_timeout_ms", &native.ConfigBluezConnectTimeout},
		{"simpleble_config_simplebluez_set_connection_timeout_ms", &native.ConfigBluezSetConnectTimeout},
		{"simpleble_config_simplebluez_get_disconnection_timeout_ms", &native.ConfigBluezDisconnectTimeout},
		{"simpleble_config_simplebluez_set_disconnection_timeout_ms", &native.ConfigBluezSetDisconnectTimeout},
		{"simpleble_config_winrt_reset", &native.ConfigWinRTReset},
		{"simpleble_config_winrt_get_experimental_use_own_mta_apartment", &native.ConfigWinRTOwnMTA},
		{"simpleble_config_winrt_set_experimental_use_own_mta_apartment", &native.ConfigWinRTSetOwnMTA},
		{"simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread", &native.ConfigWinRTReinitialize},
		{"simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread", &native.ConfigWinRTSetReinitialize},
		{"simpleble_config_winrt_get_use_deferred_disconnect", &native.ConfigWinRTDeferredDisconnect},
		{"simpleble_config_winrt_set_use_deferred_disconnect", &native.ConfigWinRTSetDeferredDisconnect},
		{"simpleble_config_corebluetooth_reset", &native.ConfigCoreBluetoothReset},
		{"simpleble_config_dongl_reset", &native.ConfigDonglReset},
		{"simpleble_config_dongl_get_use_dongl_backend", &native.ConfigDonglBackend},
		{"simpleble_config_dongl_set_use_dongl_backend", &native.ConfigDonglSetBackend},
		{"simpleble_config_dongl_get_auto_update", &native.ConfigDonglAutoUpdate},
		{"simpleble_config_dongl_set_auto_update", &native.ConfigDonglSetAutoUpdate},
		{"simpleble_config_dongl_get_force_update", &native.ConfigDonglForceUpdate},
		{"simpleble_config_dongl_set_force_update", &native.ConfigDonglSetForceUpdate},
		{"simpleble_advanced_dongl_set_passkey_request_callback", &native.DonglPasskeyRequest},
		{"simpleble_advanced_dongl_set_passkey_display_callback", &native.DonglPasskeyDisplay},
		{"simpleble_advanced_dongl_set_numeric_comparison_callback", &native.DonglNumericComparison},
	}
	for _, binding := range bindings {
		symbol, err := findSymbol(libraryHandle, binding.name)
		if err != nil {
			closeLibrary(libraryHandle)
			return fmt.Errorf("simplegoble: incompatible SimpleCBLE library %q: %s: %w", path, binding.name, err)
		}
		purego.RegisterFunc(binding.target, symbol)
	}
	// NOTE: Platform functions only exist in libraries built with that platform's
	// backend, so a missing symbol just leaves its function nil.
	if platform := map[string]string{"darwin": "macos", "linux": "linux"}[runtime.GOOS]; platform != "" {
		for _, binding := range []struct {
			name   string
			target any
		}{
			{"simpleble_advanced_" + platform + "_set_advertisement_local_name", &native.SetAdvertisementLocalName},
			{"simpleble_advanced_" + platform + "_retrieve_cached_peripheral", &native.RetrieveCachedPeripheral},
		} {
			if symbol, err := findSymbol(libraryHandle, binding.name); err == nil {
				purego.RegisterFunc(binding.target, symbol)
			}
		}
	}
	loadedLibrary.api, loadedLibrary.path = native, path
	return nil
}

func API() (*NativeAPI, error) {
	if err := Load(""); err != nil {
		return nil, err
	}
	loadedLibrary.Lock()
	defer loadedLibrary.Unlock()
	return loadedLibrary.api, nil
}

// Version returns the loaded native SimpleBLE version.
func Version() (string, error) {
	native, err := API()
	if err != nil {
		return "", err
	}
	return native.Version(), nil
}

func (native *NativeAPI) TakeError(nativeError *uintptr) error {
	if *nativeError == 0 {
		return nil
	}
	defer native.ErrorRelease(nativeError)
	return &Error{Code: ErrorCode(native.ErrorCode(*nativeError)), Message: native.ErrorMessage(*nativeError)}
}

func (native *NativeAPI) TakeString(nativePointer *byte) string {
	if nativePointer == nil {
		return ""
	}
	defer native.Free(unsafe.Pointer(nativePointer))
	var length int
	for *(*byte)(unsafe.Add(unsafe.Pointer(nativePointer), length)) != 0 {
		length++
	}
	return string(unsafe.Slice(nativePointer, length))
}
