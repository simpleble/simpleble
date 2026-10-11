package simplegoble

import (
	"sync"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// LocalService is an owned local GATT service handle.
type LocalService struct {
	handle *bindings.Handle
	state  *localServiceState
}

type localServiceState struct {
	sync.Mutex
	readers map[string]*bindings.ReadCallback
}

func newLocalService(native *bindings.NativeAPI, nativeHandle uintptr, state *localServiceState) *LocalService {
	return &LocalService{handle: &bindings.Handle{Native: native, NativeHandle: nativeHandle}, state: state}
}

// Initialized reports whether the handle is open.
func (s *LocalService) Initialized() bool { return s.handle.Initialized() }

// Close releases this handle. It does not remove the service from the host.
func (s *LocalService) Close() error { return bindings.CloseLocalService(s.handle) }

// UUID returns the normalized service UUID.
func (s *LocalService) UUID() (string, error) {
	return bindings.Query(s.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		var uuid bindings.UUID
		native.LocalServiceUUID(nativeHandle, &uuid, nativeError)
		return uuid.String()
	})
}

// AddCharacteristic adds a characteristic before the host starts.
// Combine capabilities with bitwise OR. Close the returned handle when finished.
// Characteristic UUIDs must be unique within the service.
func (s *LocalService) AddCharacteristic(value string, capabilities CharacteristicCapability) (*LocalCharacteristic, error) {
	uuid, err := bindings.NewUUID(value)
	if err != nil {
		return nil, err
	}
	if s.handle == nil {
		return nil, ErrClosed
	}
	s.state.Lock()
	defer s.state.Unlock()
	if s.state.readers[uuid.String()] != nil {
		return nil, &bindings.Error{Code: bindings.InvalidArgument, Message: "duplicate local characteristic UUID"}
	}
	return bindings.Query(s.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalCharacteristic {
		handle := native.LocalServiceAddCharacteristic(nativeHandle, uuid, uint32(capabilities), nativeError)
		if *nativeError != 0 {
			return nil
		}
		reader := &bindings.ReadCallback{}
		s.state.readers[uuid.String()] = reader
		return newLocalCharacteristic(native, handle, reader)
	})
}

// Characteristics returns independent owned handles. Close each when finished.
func (s *LocalService) Characteristics() ([]*LocalCharacteristic, error) {
	if s.handle == nil {
		return nil, ErrClosed
	}
	s.state.Lock()
	defer s.state.Unlock()
	return bindings.Query(s.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []*LocalCharacteristic {
		count := native.LocalServiceCharacteristicCount(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		characteristics := make([]*LocalCharacteristic, 0, count)
		for i := uintptr(0); i < count; i++ {
			handle := native.LocalServiceCharacteristicGet(nativeHandle, i, nativeError)
			var uuid bindings.UUID
			if *nativeError == 0 {
				native.LocalCharacteristicUUID(handle, &uuid, nativeError)
			}
			if *nativeError != 0 {
				native.LocalCharacteristicRelease(handle)
				for _, c := range characteristics {
					_ = c.Close()
				}
				return nil
			}
			reader := s.state.readers[uuid.String()]
			if reader == nil {
				reader = &bindings.ReadCallback{}
				s.state.readers[uuid.String()] = reader
			}
			characteristics = append(characteristics, newLocalCharacteristic(native, handle, reader))
		}
		return characteristics
	})
}
