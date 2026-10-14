package simplegoble

import "github.com/simpleble/simplegoble/internal/bindings"

// Backend is an owned SimpleBLE backend handle. Close every returned Backend;
// adapters obtained from it have independent lifetimes.
type Backend struct{ handle *bindings.Handle }

// GetBackends initializes and returns all available backends.
func GetBackends() ([]*Backend, error) {
	native, err := bindings.API()
	if err != nil {
		return nil, err
	}
	var nativeError uintptr
	count := native.BackendCount(&nativeError)
	if err := native.TakeError(&nativeError); err != nil {
		return nil, err
	}
	backends := make([]*Backend, 0, count)
	for i := uintptr(0); i < count; i++ {
		nativeHandle := native.BackendGet(i, &nativeError)
		if err := native.TakeError(&nativeError); err != nil {
			for _, b := range backends {
				_ = b.Close()
			}
			return nil, err
		}
		backends = append(backends, &Backend{handle: bindings.NewBackend(native, nativeHandle)})
	}
	return backends, nil
}

// GetNativeBackend returns the backend of the operating system's Bluetooth stack.
func GetNativeBackend() (*Backend, error) {
	return namedBackend(func(native *bindings.NativeAPI, nativeError *uintptr) uintptr {
		return native.BackendNative(nativeError)
	})
}

// GetDonglBackend returns the Dongl backend, whether or not
// Config.Dongl.UseDonglBackend lists it in GetBackends. It fails with
// OperationNotSupported if this build of SimpleBLE doesn't include it.
func GetDonglBackend() (*Backend, error) {
	return namedBackend(func(native *bindings.NativeAPI, nativeError *uintptr) uintptr {
		return native.BackendDongl(nativeError)
	})
}

// GetSimulatorBackend returns the Simulator backend, which only has adapters
// while a simulation is active. It fails with OperationNotSupported if this
// build of SimpleBLE doesn't include it.
func GetSimulatorBackend() (*Backend, error) {
	return namedBackend(func(native *bindings.NativeAPI, nativeError *uintptr) uintptr {
		return native.BackendSimulator(nativeError)
	})
}

func namedBackend(get func(native *bindings.NativeAPI, nativeError *uintptr) uintptr) (*Backend, error) {
	native, err := bindings.API()
	if err != nil {
		return nil, err
	}
	var nativeError uintptr
	nativeHandle := get(native, &nativeError)
	if err := native.TakeError(&nativeError); err != nil {
		return nil, err
	}
	return &Backend{handle: bindings.NewBackend(native, nativeHandle)}, nil
}

// Initialized reports whether the handle is open.
func (b *Backend) Initialized() bool { return b.handle.Initialized() }

// Close releases this handle. It is idempotent and does not close its adapters.
func (b *Backend) Close() error { return bindings.CloseBackend(b.handle) }

// Identifier returns the native backend name.
func (b *Backend) Identifier() (string, error) {
	return bindings.Query(b.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		return native.TakeString(native.BackendIdentifier(nativeHandle, nativeError))
	})
}

// BluetoothEnabled reports whether Bluetooth is enabled for this backend.
func (b *Backend) BluetoothEnabled() (bool, error) {
	return bindings.Query(b.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.BackendEnabled(nativeHandle, nativeError)
	})
}

// Adapters returns independently owned handles. Close every returned Adapter.
func (b *Backend) Adapters() ([]*Adapter, error) {
	return bindings.Query(b.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []*Adapter {
		count := native.BackendAdapterCount(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		return collectAdapters(native, count, func(i uintptr) uintptr { return native.BackendAdapterGet(nativeHandle, i, nativeError) }, nativeError)
	})
}
