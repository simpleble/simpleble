package bindings

import (
	"fmt"
	"sync"
	"unsafe"

	"github.com/ebitengine/purego"
)

// Keep C pointers opaque, use fixed-width C int/enum types, and pass actual Go
// pointers for synchronous out parameters so purego keeps them alive.
type NativeAPI struct {
	Free                        func(unsafe.Pointer)
	Version                     func() string
	ErrorCode                   func(uintptr) uint32
	ErrorMessage                func(uintptr) string
	ErrorRelease                func(*uintptr)
	BackendCount                func(*uintptr) uintptr
	BackendGet                  func(uintptr, *uintptr) uintptr
	BackendRelease              func(uintptr)
	BackendIdentifier           func(uintptr, *uintptr) *byte
	BackendEnabled              func(uintptr, *uintptr) bool
	BackendAdapterCount         func(uintptr, *uintptr) uintptr
	BackendAdapterGet           func(uintptr, uintptr, *uintptr) uintptr
	AdapterCount                func(*uintptr) uintptr
	AdapterGet                  func(uintptr, *uintptr) uintptr
	AdapterRelease              func(uintptr)
	AdapterEnabled              func(*uintptr) bool
	AdapterIdentifier           func(uintptr, *uintptr) *byte
	AdapterAddress              func(uintptr, *uintptr) *byte
	AdapterPowerOn              func(uintptr, *uintptr)
	AdapterPowerOff             func(uintptr, *uintptr)
	AdapterPowered              func(uintptr, *uintptr) bool
	AdapterScanStart            func(uintptr, *uintptr)
	AdapterScanStop             func(uintptr, *uintptr)
	AdapterScanFor              func(uintptr, int32, *uintptr)
	AdapterScanActive           func(uintptr, *uintptr) bool
	SetCallbacks                [4]func(uintptr, uintptr, uintptr)
	AdapterScanCount            func(uintptr, *uintptr) uintptr
	AdapterScanGet              func(uintptr, uintptr, *uintptr) uintptr
	AdapterPairedCount          func(uintptr, *uintptr) uintptr
	AdapterPairedGet            func(uintptr, uintptr, *uintptr) uintptr
	AdapterConnectedCount       func(uintptr, *uintptr) uintptr
	AdapterConnectedGet         func(uintptr, uintptr, *uintptr) uintptr
	AdapterScanFound            func(uintptr, uintptr, uintptr)
	AdapterScanUpdated          func(uintptr, uintptr, uintptr)
	PeripheralRelease           func(uintptr)
	PeripheralIdentifier        func(uintptr, *uintptr) *byte
	PeripheralAddress           func(uintptr, *uintptr) *byte
	PeripheralAddressType       func(uintptr, *uintptr) uint32
	PeripheralRSSI              func(uintptr, *uintptr) int16
	PeripheralTxPower           func(uintptr, *uintptr) int16
	PeripheralMTU               func(uintptr, *uintptr) uint16
	PeripheralConnect           func(uintptr, *uintptr)
	PeripheralDisconnect        func(uintptr, *uintptr)
	PeripheralConnected         func(uintptr, *uintptr) bool
	PeripheralConnectable       func(uintptr, *uintptr) bool
	PeripheralPaired            func(uintptr, *uintptr) bool
	PeripheralUnpair            func(uintptr, *uintptr)
	PeripheralServiceCount      func(uintptr, *uintptr) uintptr
	PeripheralServiceGet        func(uintptr, uintptr, *Service, *uintptr)
	ServiceRelease              func(*Service)
	PeripheralManufacturerCount func(uintptr, *uintptr) uintptr
	PeripheralManufacturerGet   func(uintptr, uintptr, *ManufacturerData, *uintptr)
	ManufacturerRelease         func(*ManufacturerData)
	PeripheralRead              func(uintptr, UUID, UUID, *uintptr, *uintptr) *byte
	PeripheralReadDescriptor    func(uintptr, UUID, UUID, UUID, *uintptr, *uintptr) *byte
	PeripheralWriteRequest      func(uintptr, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralWriteCommand      func(uintptr, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralWriteDescriptor   func(uintptr, UUID, UUID, UUID, *byte, uintptr, *uintptr)
	PeripheralNotify            func(uintptr, UUID, UUID, uintptr, uintptr, *uintptr)
	PeripheralIndicate          func(uintptr, UUID, UUID, uintptr, uintptr, *uintptr)
	PeripheralUnsubscribe       func(uintptr, UUID, UUID, *uintptr)
	PeripheralOnConnected       func(uintptr, uintptr, uintptr)
	PeripheralOnDisconnected    func(uintptr, uintptr, uintptr)
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
		{"simpleble_backend_get_count", &native.BackendCount},
		{"simpleble_backend_get_handle", &native.BackendGet},
		{"simpleble_backend_release_handle", &native.BackendRelease},
		{"simpleble_backend_identifier", &native.BackendIdentifier},
		{"simpleble_backend_is_bluetooth_enabled", &native.BackendEnabled},
		{"simpleble_backend_get_adapters_count", &native.BackendAdapterCount},
		{"simpleble_backend_get_adapters_handle", &native.BackendAdapterGet},
		{"simpleble_adapter_get_count", &native.AdapterCount},
		{"simpleble_adapter_get_handle", &native.AdapterGet},
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
		{"simpleble_adapter_scan_get_results_count", &native.AdapterScanCount},
		{"simpleble_adapter_scan_get_results_handle", &native.AdapterScanGet},
		{"simpleble_adapter_get_paired_peripherals_count", &native.AdapterPairedCount},
		{"simpleble_adapter_get_paired_peripherals_handle", &native.AdapterPairedGet},
		{"simpleble_adapter_get_connected_peripherals_count", &native.AdapterConnectedCount},
		{"simpleble_adapter_get_connected_peripherals_handle", &native.AdapterConnectedGet},
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
		{"simpleble_peripheral_services_count", &native.PeripheralServiceCount},
		{"simpleble_peripheral_services_get", &native.PeripheralServiceGet},
		{"simpleble_service_release", &native.ServiceRelease},
		{"simpleble_peripheral_manufacturer_data_count", &native.PeripheralManufacturerCount},
		{"simpleble_peripheral_manufacturer_data_get", &native.PeripheralManufacturerGet},
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
	}
	for _, binding := range bindings {
		symbol, err := findSymbol(libraryHandle, binding.name)
		if err != nil {
			closeLibrary(libraryHandle)
			return fmt.Errorf("simplegoble: incompatible SimpleCBLE library %q: %s: %w", path, binding.name, err)
		}
		purego.RegisterFunc(binding.target, symbol)
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
