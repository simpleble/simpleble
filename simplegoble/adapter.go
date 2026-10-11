package simplegoble

import (
	"time"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// Adapter is an owned Bluetooth adapter handle. Repeated enumeration returns
// independent handles sharing the native adapter's state and callback slots.
type Adapter struct {
	handle    *bindings.Handle
	callbacks *bindings.CallbackOwner
}

func newAdapter(native *bindings.NativeAPI, nativeHandle uintptr) *Adapter {
	return &Adapter{handle: &bindings.Handle{Native: native, NativeHandle: nativeHandle}, callbacks: &bindings.CallbackOwner{}}
}

// GetAdapters initializes backends and returns their adapters. Close all handles.
func GetAdapters() ([]*Adapter, error) {
	native, err := bindings.API()
	if err != nil {
		return nil, err
	}
	var nativeError uintptr
	count := native.AdapterCount(&nativeError)
	if err := native.TakeError(&nativeError); err != nil {
		return nil, err
	}
	adapters := collectAdapters(native, count, func(i uintptr) uintptr { return native.AdapterGet(i, &nativeError) }, &nativeError)
	return adapters, native.TakeError(&nativeError)
}

func collectAdapters(native *bindings.NativeAPI, count uintptr, get func(uintptr) uintptr, nativeError *uintptr) []*Adapter {
	adapters := make([]*Adapter, 0, count)
	for i := uintptr(0); i < count; i++ {
		nativeHandle := get(i)
		if *nativeError != 0 {
			for _, a := range adapters {
				_ = a.Close()
			}
			return nil
		}
		adapters = append(adapters, newAdapter(native, nativeHandle))
	}
	return adapters
}

// BluetoothEnabled reports whether any backend has Bluetooth enabled.
func BluetoothEnabled() (bool, error) {
	native, err := bindings.API()
	if err != nil {
		return false, err
	}
	var nativeError uintptr
	enabled := native.AdapterEnabled(&nativeError)
	return enabled, native.TakeError(&nativeError)
}

// Initialized reports whether the handle is open.
func (a *Adapter) Initialized() bool { return a.handle.Initialized() }

// Close detaches this wrapper's Go callbacks and releases its native handle.
// It waits for native calls, but not for an already selected Go callback, and
// can be called from a callback. Scanning and power state are unaffected.
func (a *Adapter) Close() error { return bindings.CloseAdapter(a.handle, a.callbacks) }

// Identifier returns the adapter's native name.
func (a *Adapter) Identifier() (string, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		return native.TakeString(native.AdapterIdentifier(nativeHandle, nativeError))
	})
}

// Address returns the adapter's Bluetooth address.
func (a *Adapter) Address() (string, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		return native.TakeString(native.AdapterAddress(nativeHandle, nativeError))
	})
}

// PowerOn requests power on. Unsupported backends may do nothing.
func (a *Adapter) PowerOn() error {
	return bindings.Command(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.AdapterPowerOn(nativeHandle, nativeError)
	})
}

// PowerOff requests power off. Unsupported backends may do nothing.
func (a *Adapter) PowerOff() error {
	return bindings.Command(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.AdapterPowerOff(nativeHandle, nativeError)
	})
}

// IsPowered reports the adapter's power state.
func (a *Adapter) IsPowered() (bool, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.AdapterPowered(nativeHandle, nativeError)
	})
}

// ScanStart starts scanning.
func (a *Adapter) ScanStart() error {
	return bindings.Command(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.AdapterScanStart(nativeHandle, nativeError)
	})
}

// ScanStop stops scanning.
func (a *Adapter) ScanStop() error {
	return bindings.Command(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.AdapterScanStop(nativeHandle, nativeError)
	})
}

// ScanIsActive reports whether scanning is active.
func (a *Adapter) ScanIsActive() (bool, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.AdapterScanActive(nativeHandle, nativeError)
	})
}

// ScanFor scans synchronously for duration. Other goroutines remain runnable.
// Positive fractional milliseconds round up. The duration must fit a
// nonnegative signed 32-bit millisecond count. The native call is not cancellable;
// use ScanStart and ScanStop when the application must control scan lifetime.
func (a *Adapter) ScanFor(duration time.Duration) error {
	if duration < 0 || duration > time.Duration(1<<31-1)*time.Millisecond {
		return &Error{Code: InvalidArgument, Message: "scan duration must fit a nonnegative 32-bit millisecond count"}
	}
	ms := int32((duration + time.Millisecond - 1) / time.Millisecond)
	return bindings.Command(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.AdapterScanFor(nativeHandle, ms, nativeError)
	})
}

// ScanGetResults returns independent owned handles for the current scan results.
// Close each returned peripheral handle when finished.
func (a *Adapter) ScanGetResults() ([]*Peripheral, error) {
	if a.handle == nil {
		return nil, ErrClosed
	}
	return a.peripherals(a.handle.Native.AdapterScanCount, a.handle.Native.AdapterScanGet)
}

// GetPairedPeripherals returns owned handles for paired devices.
func (a *Adapter) GetPairedPeripherals() ([]*Peripheral, error) {
	if a.handle == nil {
		return nil, ErrClosed
	}
	return a.peripherals(a.handle.Native.AdapterPairedCount, a.handle.Native.AdapterPairedGet)
}

// GetConnectedPeripherals returns owned handles for connected devices.
func (a *Adapter) GetConnectedPeripherals() ([]*Peripheral, error) {
	if a.handle == nil {
		return nil, ErrClosed
	}
	return a.peripherals(a.handle.Native.AdapterConnectedCount, a.handle.Native.AdapterConnectedGet)
}

func (a *Adapter) peripherals(count func(uintptr, *uintptr) uintptr, get func(uintptr, uintptr, *uintptr) uintptr) ([]*Peripheral, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []*Peripheral {
		length := count(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		peripherals := make([]*Peripheral, 0, length)
		for i := uintptr(0); i < length; i++ {
			peripheralHandle := get(nativeHandle, i, nativeError)
			if *nativeError != 0 {
				for _, p := range peripherals {
					_ = p.Close()
				}
				return nil
			}
			peripherals = append(peripherals, newPeripheral(native, peripheralHandle))
		}
		return peripherals
	})
}

// SetCallbackOnPowerOn replaces the native power-on callback; nil clears it.
// Callbacks run in order on this wrapper's Go worker and may call its methods
// or Close. Return promptly; slow handlers delay later callbacks. Panics follow
// normal Go behavior. Native callback slots are shared across handles for the
// same adapter: the latest registration wins.
func (a *Adapter) SetCallbackOnPowerOn(fn func()) error {
	return bindings.SetCallback(a.handle, a.callbacks, bindings.PowerOn, fn)
}

// SetCallbackOnPowerOff replaces the power-off callback; nil clears it.
// Dispatch and ownership follow SetCallbackOnPowerOn.
func (a *Adapter) SetCallbackOnPowerOff(fn func()) error {
	return bindings.SetCallback(a.handle, a.callbacks, bindings.PowerOff, fn)
}

// SetCallbackOnScanStart replaces the scan-start callback; nil clears it.
// Dispatch and ownership follow SetCallbackOnPowerOn.
func (a *Adapter) SetCallbackOnScanStart(fn func()) error {
	return bindings.SetCallback(a.handle, a.callbacks, bindings.ScanStart, fn)
}

// SetCallbackOnScanStop replaces the scan-stop callback; nil clears it.
// Dispatch and ownership follow SetCallbackOnPowerOn.
func (a *Adapter) SetCallbackOnScanStop(fn func()) error {
	return bindings.SetCallback(a.handle, a.callbacks, bindings.ScanStop, fn)
}

// SetCallbackOnScanFound replaces the discovery callback; nil clears it.
// Each invocation transfers an owned peripheral handle that the handler must
// Close. Dispatch follows SetCallbackOnPowerOn. Retaining the handle is allowed.
func (a *Adapter) SetCallbackOnScanFound(fn func(*Peripheral)) error {
	return a.setPeripheralCallback(bindings.ScanFound, fn)
}

// SetCallbackOnScanUpdated replaces the advertisement update callback.
// Each event transfers its own handle, following SetCallbackOnScanFound.
func (a *Adapter) SetCallbackOnScanUpdated(fn func(*Peripheral)) error {
	return a.setPeripheralCallback(bindings.ScanUpdated, fn)
}

func (a *Adapter) setPeripheralCallback(kind int, fn func(*Peripheral)) error {
	var callback func(uintptr)
	if fn != nil {
		callback = func(nativeHandle uintptr) { fn(newPeripheral(a.handle.Native, nativeHandle)) }
	}
	return bindings.SetPeripheralCallback(a.handle, a.callbacks, kind, callback)
}
