package simplegoble

import (
	"sync"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// LocalPeripheral hosts GATT services on this adapter. Close it when finished.
type LocalPeripheral struct {
	handle    *bindings.Handle
	callbacks *bindings.CallbackOwner
	state     *localPeripheralState
}

type localPeripheralState struct {
	sync.Mutex
	services map[string]*localServiceState
}

// CreateLocalPeripheral creates an owned GATT host. Configure it before Start.
func (a *Adapter) CreateLocalPeripheral() (*LocalPeripheral, error) {
	return bindings.Query(a.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalPeripheral {
		host := native.AdapterCreateLocalPeripheral(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		return &LocalPeripheral{handle: &bindings.Handle{Native: native, NativeHandle: host},
			callbacks: &bindings.CallbackOwner{}, state: &localPeripheralState{services: make(map[string]*localServiceState)}}
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

// AddService adds a primary GATT service before Start and returns an owned handle.
// Service UUIDs must be unique within the host.
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
	if p.state.services[uuid.String()] != nil {
		return nil, &bindings.Error{Code: bindings.InvalidArgument, Message: "duplicate local service UUID"}
	}
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalService {
		handle := native.LocalPeripheralAddService(nativeHandle, uuid, nativeError)
		if *nativeError != 0 {
			return nil
		}
		state := &localServiceState{readers: make(map[string]*bindings.ReadCallback)}
		p.state.services[uuid.String()] = state
		return newLocalService(native, handle, state)
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
		count := native.LocalPeripheralServiceCount(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		services := make([]*LocalService, 0, count)
		for i := uintptr(0); i < count; i++ {
			handle := native.LocalPeripheralServiceGet(nativeHandle, i, nativeError)
			var uuid bindings.UUID
			if *nativeError == 0 {
				native.LocalServiceUUID(handle, &uuid, nativeError)
			}
			if *nativeError != 0 {
				native.LocalServiceRelease(handle)
				for _, service := range services {
					_ = service.Close()
				}
				return nil
			}
			state := p.state.services[uuid.String()]
			if state == nil {
				state = &localServiceState{readers: make(map[string]*bindings.ReadCallback)}
				p.state.services[uuid.String()] = state
			}
			services = append(services, newLocalService(native, handle, state))
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
		clear(p.state.services)
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
