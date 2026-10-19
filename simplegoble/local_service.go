package simplegoble

import "github.com/simpleble/simplegoble/internal/bindings"

// LocalService is an owned local GATT service handle.
type LocalService struct {
	handle *bindings.Handle
	state  *localChildren[bindings.ReadCallback]
}

func newLocalService(native *bindings.NativeAPI, nativeHandle uintptr, state *localChildren[bindings.ReadCallback]) *LocalService {
	return &LocalService{handle: bindings.NewLocalService(native, nativeHandle), state: state}
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
// Whether a service accepts repeated characteristic UUIDs depends on the backend.
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
	return bindings.Query(s.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) *LocalCharacteristic {
		handle := native.LocalServiceAddCharacteristic(nativeHandle, uuid, uint32(capabilities), nativeError)
		if *nativeError != 0 {
			return nil
		}
		return newLocalCharacteristic(native, handle, s.state.add(uuid.String()))
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
		var list uintptr
		count := native.LocalServiceCharacteristics(nativeHandle, &list, nativeError)
		if *nativeError != 0 {
			return nil
		}
		defer native.LocalCharacteristicListRelease(list)
		characteristics := make([]*LocalCharacteristic, 0, count)
		seen := make(map[string]int)
		for i := uintptr(0); i < count; i++ {
			handle := native.LocalCharacteristicListGet(list, i, nativeError)
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
			characteristics = append(characteristics, newLocalCharacteristic(native, handle, s.state.at(uuid.String(), seen)))
		}
		return characteristics
	})
}
