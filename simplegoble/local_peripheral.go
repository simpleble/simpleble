package simplegoble

import (
	"sync"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// LocalPeripheral hosts GATT services on this adapter. Close it when finished.
type LocalPeripheral struct {
	handle    *bindings.Handle
	callbacks *bindings.CallbackOwner
	state     *localChildren[localChildren[bindings.ReadCallback]]
}

// localChildren keeps Go state for local GATT children by their native identity,
// which every handle to the same child shares.
type localChildren[T any] struct {
	sync.Mutex
	byIdentity map[uintptr]*T
}

// at returns state for the child with identity, creating it on first use.
func (c *localChildren[T]) at(identity uintptr) *T {
	if c.byIdentity == nil {
		c.byIdentity = make(map[uintptr]*T)
	}
	state := c.byIdentity[identity]
	if state == nil {
		state = new(T)
		c.byIdentity[identity] = state
	}
	return state
}

// CreateLocalPeripheral creates an owned GATT host. Configure it before Start.
func (a *Adapter) CreateLocalPeripheral() (*LocalPeripheral, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalPeripheral {
		host := native.AdapterCreateLocalPeripheral(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		return &LocalPeripheral{handle: bindings.NewLocalPeripheral(native, host),
			callbacks: &bindings.CallbackOwner{}, state: &localChildren[localChildren[bindings.ReadCallback]]{}}
	})
}

// Initialized reports whether the handle is open.
func (p *LocalPeripheral) Initialized() bool { return p.handle.Initialized() }

// Close expires this wrapper's callbacks and releases the host, stopping it.
// Close services and characteristics separately. Copies share ownership.
func (p *LocalPeripheral) Close() error { return bindings.CloseLocalPeripheral(p.handle, p.callbacks) }

// AddAdvertisedService adds a service UUID to the advertisement before Start.
func (p *LocalPeripheral) AddAdvertisedService(value string) error {
	uuid, err := bindings.NewUUID(value)
	if err != nil {
		return err
	}
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralAdvertiseService(nativeHandle, uuid, nativeError)
	})
}

// SetAdvertisedManufacturerData adds manufacturer specific data to the advertisement before Start.
// Setting data for the same company identifier again replaces it.
func (p *LocalPeripheral) SetAdvertisedManufacturerData(companyID uint16, data []byte) error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralManufacturerData(nativeHandle, companyID, bindings.InputBytes(data), uintptr(len(data)),
			nativeError)
	})
}

// SetConnectable chooses whether centrals can connect, before Start. Local peripherals are connectable by default.
func (p *LocalPeripheral) SetConnectable(connectable bool) error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralSetConnectable(nativeHandle, connectable, nativeError)
	})
}

// AddService adds a primary GATT service before Start and returns an owned handle.
// Whether a host accepts repeated service UUIDs depends on the backend.
func (p *LocalPeripheral) AddService(value string) (*LocalService, error) {
	uuid, err := bindings.NewUUID(value)
	if err != nil {
		return nil, err
	}
	if p.handle == nil {
		return nil, ErrClosed
	}
	p.state.Lock()
	defer p.state.Unlock()
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalService {
		handle := native.LocalPeripheralAddService(nativeHandle, uuid, nativeError)
		if *nativeError != 0 {
			return nil
		}
		return newLocalService(native, handle, p.state.at(native.LocalServiceIdentity(handle)))
	})
}

// Services returns independent owned handles. Close each when finished.
func (p *LocalPeripheral) Services() ([]*LocalService, error) {
	if p.handle == nil {
		return nil, ErrClosed
	}
	p.state.Lock()
	defer p.state.Unlock()
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []*LocalService {
		var list uintptr
		count := native.LocalPeripheralServices(nativeHandle, &list, nativeError)
		if *nativeError != 0 {
			return nil
		}
		defer native.LocalServiceListRelease(list)
		services := make([]*LocalService, 0, count)
		for i := uintptr(0); i < count; i++ {
			handle := native.LocalServiceListGet(list, i, nativeError)
			if *nativeError != 0 {
				for _, service := range services {
					_ = service.Close()
				}
				return nil
			}
			services = append(services, newLocalService(native, handle, p.state.at(native.LocalServiceIdentity(handle))))
		}
		return services
	})
}

// RemoveAllServices removes GATT services before Start.
func (p *LocalPeripheral) RemoveAllServices() error {
	if p.handle == nil {
		return ErrClosed
	}
	p.state.Lock()
	defer p.state.Unlock()
	err := bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralRemoveServices(nativeHandle, nativeError)
	})
	if err == nil {
		p.state.byIdentity = nil
	}
	return err
}

// Start publishes the configured services and starts advertising.
func (p *LocalPeripheral) Start() error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralStart(nativeHandle, nativeError)
	})
}

// Stop stops hosting and advertising.
func (p *LocalPeripheral) Stop() error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalPeripheralStop(nativeHandle, nativeError)
	})
}

// IsStarted reports whether the host is running.
func (p *LocalPeripheral) IsStarted() (bool, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.LocalPeripheralStarted(nativeHandle, nativeError)
	})
}

// IsAdvertising reports whether the host is advertising.
func (p *LocalPeripheral) IsAdvertising() (bool, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.LocalPeripheralAdvertising(nativeHandle, nativeError)
	})
}

// SetCallbackOnClientConnected observes client connections; nil clears it.
// Addresses are Go-owned. Handlers run serially and may call host methods or Close.
func (p *LocalPeripheral) SetCallbackOnClientConnected(fn func(string)) error {
	return bindings.SetLocalAddressCallback(p.handle, p.callbacks, bindings.LocalConnected, fn)
}

// SetCallbackOnClientDisconnected observes client disconnections; nil clears it.
// Dispatch follows SetCallbackOnClientConnected.
func (p *LocalPeripheral) SetCallbackOnClientDisconnected(fn func(string)) error {
	return bindings.SetLocalAddressCallback(p.handle, p.callbacks, bindings.LocalDisconnected, fn)
}
