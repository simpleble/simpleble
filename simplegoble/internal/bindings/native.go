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
	Free                func(unsafe.Pointer)
	Version             func() string
	ErrorCode           func(uintptr) uint32
	ErrorMessage        func(uintptr) string
	ErrorRelease        func(*uintptr)
	BackendCount        func(*uintptr) uintptr
	BackendGet          func(uintptr, *uintptr) uintptr
	BackendRelease      func(uintptr)
	BackendIdentifier   func(uintptr, *uintptr) *byte
	BackendEnabled      func(uintptr, *uintptr) bool
	BackendAdapterCount func(uintptr, *uintptr) uintptr
	BackendAdapterGet   func(uintptr, uintptr, *uintptr) uintptr
	AdapterCount        func(*uintptr) uintptr
	AdapterGet          func(uintptr, *uintptr) uintptr
	AdapterRelease      func(uintptr)
	AdapterEnabled      func(*uintptr) bool
	AdapterIdentifier   func(uintptr, *uintptr) *byte
	AdapterAddress      func(uintptr, *uintptr) *byte
	AdapterPowerOn      func(uintptr, *uintptr)
	AdapterPowerOff     func(uintptr, *uintptr)
	AdapterPowered      func(uintptr, *uintptr) bool
	AdapterScanStart    func(uintptr, *uintptr)
	AdapterScanStop     func(uintptr, *uintptr)
	AdapterScanFor      func(uintptr, int32, *uintptr)
	AdapterScanActive   func(uintptr, *uintptr) bool
	SetCallbacks        [4]func(uintptr, uintptr, uintptr)
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
