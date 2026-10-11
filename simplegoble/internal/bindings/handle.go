package bindings

import "sync"

// Wrappers contain a pointer to this state, so copying them cannot double-free.
// Native calls share the read lock, so a blocking ScanFor does not stall other
// calls; Close takes the write lock and waits for calls in progress.
type Handle struct {
	sync.RWMutex
	Native       *NativeAPI
	NativeHandle uintptr
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
		callbacks.close()
		handle.Native.AdapterRelease(handle.NativeHandle)
		handle.NativeHandle = 0
	}
	return nil
}
