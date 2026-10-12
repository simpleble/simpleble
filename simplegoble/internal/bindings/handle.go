package bindings

import (
	"runtime"
	"sync"
)

// Wrappers contain a pointer to this state, so copying them cannot double-free.
// Native calls share the read lock, so a blocking ScanFor does not stall other
// calls; Close takes the write lock and waits for calls in progress.
type Handle struct {
	sync.RWMutex
	Native       *NativeAPI
	NativeHandle uintptr
	cleanup      runtime.Cleanup
}

// Like os.File, a tracked handle releases its native object once every wrapper
// copy is unreachable without Close, and Close stops the cleanup. Registered
// callbacks keep their handle reachable. release must not reference the handle.
func track(native *NativeAPI, nativeHandle uintptr, release func()) *Handle {
	handle := &Handle{Native: native, NativeHandle: nativeHandle}
	handle.cleanup = runtime.AddCleanup(handle, func(release func()) { release() }, release)
	return handle
}

func NewBackend(native *NativeAPI, nativeHandle uintptr) *Handle {
	return track(native, nativeHandle, func() { native.BackendRelease(nativeHandle) })
}

func NewAdapter(native *NativeAPI, nativeHandle uintptr, callbacks *CallbackOwner) *Handle {
	return track(native, nativeHandle, func() { callbacks.close(); native.AdapterRelease(nativeHandle) })
}

func NewPeripheral(native *NativeAPI, nativeHandle uintptr, callbacks *CallbackOwner) *Handle {
	return track(native, nativeHandle, func() { callbacks.close(); native.PeripheralRelease(nativeHandle) })
}

// Releasing a host stops hosting, so an unreachable host is never released:
// a started host must not stop when its last wrapper goes out of scope.
func NewLocalPeripheral(native *NativeAPI, nativeHandle uintptr) *Handle {
	return &Handle{Native: native, NativeHandle: nativeHandle}
}

func NewLocalService(native *NativeAPI, nativeHandle uintptr) *Handle {
	return track(native, nativeHandle, func() { native.LocalServiceRelease(nativeHandle) })
}

// Unlike Close, the cleanup leaves the read slot installed, so a running host
// keeps serving dynamic reads after the wrapper becomes unreachable.
func NewLocalCharacteristic(native *NativeAPI, nativeHandle uintptr, callbacks *CallbackOwner) *Handle {
	return track(native, nativeHandle, func() { callbacks.close(); native.LocalCharacteristicRelease(nativeHandle) })
}

func (handle *Handle) Initialized() bool {
	if handle == nil {
		return false
	}
	handle.RLock()
	defer handle.RUnlock()
	return handle.NativeHandle != 0
}

func Query[T any](handle *Handle, fn func(*NativeAPI, uintptr, *uintptr) T) (T, error) {
	var zero T
	if handle == nil {
		return zero, ErrClosed
	}
	handle.RLock()
	defer handle.RUnlock()
	if handle.NativeHandle == 0 {
		return zero, ErrClosed
	}
	var nativeError uintptr
	result := fn(handle.Native, handle.NativeHandle, &nativeError)
	return result, handle.Native.TakeError(&nativeError)
}

func Command(handle *Handle, fn func(*NativeAPI, uintptr, *uintptr)) error {
	_, err := Query(handle, func(native *NativeAPI, nativeHandle uintptr, nativeError *uintptr) struct{} {
		fn(native, nativeHandle, nativeError)
		return struct{}{}
	})
	return err
}

func CloseBackend(handle *Handle) error {
	if handle == nil {
		return nil
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		handle.cleanup.Stop()
		handle.Native.BackendRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}

func CloseAdapter(handle *Handle, callbacks *CallbackOwner) error {
	if handle == nil {
		return nil
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		// Do not clear native slots: another handle may have replaced them.
		// Expired tokens ignore late native events without retaining Go callbacks.
		handle.cleanup.Stop()
		callbacks.close()
		handle.Native.AdapterRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}

func ClosePeripheral(handle *Handle, callbacks *CallbackOwner) error {
	if handle == nil {
		return nil
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		// Native slots may now belong to another wrapper. Expire only our tokens.
		handle.cleanup.Stop()
		callbacks.close()
		handle.Native.PeripheralRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}

func CloseLocalPeripheral(handle *Handle, callbacks *CallbackOwner) error {
	if handle == nil {
		return nil
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		callbacks.close()
		handle.Native.LocalPeripheralRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}

func CloseLocalService(handle *Handle) error {
	if handle == nil {
		return nil
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		handle.cleanup.Stop()
		handle.Native.LocalServiceRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}

func CloseLocalCharacteristic(handle *Handle, callbacks *CallbackOwner, reader *ReadCallback) error {
	if handle == nil {
		return nil
	}
	if err := checkReadMutation(); err != nil {
		return err
	}
	handle.Lock()
	defer handle.Unlock()
	if handle.NativeHandle != 0 {
		handle.cleanup.Stop()
		callbacks.close()
		reader.clear(handle)
		handle.Native.LocalCharacteristicRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}
