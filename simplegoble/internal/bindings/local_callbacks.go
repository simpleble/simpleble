package bindings

import (
	"runtime"
	"sync"
	"unsafe"

	"github.com/ebitengine/purego"
)

var localAddressTrampoline = purego.NewCallback(func(_ uintptr, address *byte, token uintptr) uintptr {
	dispatchCallback(token, CallbackPayload{Address: CopyString(address)}, nil)
	return 0
})

var localWriteTrampoline = purego.NewCallback(func(_ uintptr, data *byte, length, token uintptr) uintptr {
	dispatchCallback(token, CallbackPayload{Data: CopyBytes(data, length)}, nil)
	return 0
})

func SetLocalAddressCallback(handle *Handle, callbacks *CallbackOwner, kind int, fn func(string)) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { fn(payload.Address) }
	}
	return setCallback(handle, callbacks, callbackKey{kind: kind}, invoke, localAddressTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, _ *uintptr) {
			if kind == LocalConnected {
				native.LocalPeripheralOnConnected(h, trampoline, token)
			} else {
				native.LocalPeripheralOnDisconnected(h, trampoline, token)
			}
		})
}

func SetLocalWriteCallback(handle *Handle, callbacks *CallbackOwner, fn func([]byte)) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { fn(payload.Data) }
	}
	return setCallback(handle, callbacks, callbackKey{kind: localWrite}, invoke, localWriteTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, _ *uintptr) {
			native.LocalCharacteristicOnWrite(h, trampoline, token)
		})
}

type readEntry struct {
	fn   func() []byte
	data []byte
	pin  runtime.Pinner
}

var readRegistry = struct {
	sync.RWMutex
	entries map[uintptr]*readEntry
}{entries: make(map[uintptr]*readEntry)}

// Threads running a read handler. Handlers run synchronously on the native thread
// that requested the read, which SimpleSharpBLE tracks with a thread-static flag.
var readingThreads = struct {
	sync.Mutex
	ids map[uintptr]bool
}{ids: make(map[uintptr]bool)}

// enterRead marks thread as running a read handler; false means a nested read.
func enterRead(thread uintptr) bool {
	readingThreads.Lock()
	defer readingThreads.Unlock()
	if readingThreads.ids[thread] {
		return false
	}
	readingThreads.ids[thread] = true
	return true
}

func exitRead(thread uintptr) {
	readingThreads.Lock()
	delete(readingThreads.ids, thread)
	readingThreads.Unlock()
}

// The native slot would replace the callback that is running and strand its
// pinned buffer, so a read handler cannot replace or close a read handler.
func checkReadMutation() error {
	readingThreads.Lock()
	defer readingThreads.Unlock()
	if readingThreads.ids[currentThread()] {
		return &Error{Code: OperationNotSupported, Message: "cannot replace or close a read handler from a read callback"}
	}
	return nil
}

var localReadTrampoline = purego.NewCallback(func(_ uintptr, length *uintptr, token uintptr) uintptr {
	*length = 0
	readRegistry.RLock()
	entry := readRegistry.entries[token]
	readRegistry.RUnlock()
	if entry == nil {
		return 0
	}
	thread := currentThread()
	if !enterRead(thread) {
		return 0
	}
	defer exitRead(thread)
	// The native read slot serializes reads and includes the subsequent copy.
	// Its borrowed buffer must stay pinned until the next read or slot removal.
	entry.pin.Unpin()
	entry.data = append([]byte(nil), entry.fn()...)
	*length = uintptr(len(entry.data))
	if len(entry.data) == 0 {
		return 0
	}
	entry.pin.Pin(&entry.data[0])
	return uintptr(unsafe.Pointer(&entry.data[0]))
})

// ReadCallback is shared by independent wrappers of one local characteristic.
// Installation waits for the native slot before releasing the previous buffer.
type ReadCallback struct {
	sync.Mutex
	owner *Handle
	token uintptr
}

func (reader *ReadCallback) remove() {
	readRegistry.Lock()
	entry := readRegistry.entries[reader.token]
	delete(readRegistry.entries, reader.token)
	readRegistry.Unlock()
	if entry != nil {
		entry.pin.Unpin()
	}
	reader.owner, reader.token = nil, 0
}

func (reader *ReadCallback) clear(handle *Handle) {
	reader.Lock()
	defer reader.Unlock()
	if reader.owner != handle {
		return
	}
	handle.Native.LocalCharacteristicOnRead(handle.NativeHandle, 0, 0)
	reader.remove()
}

func SetLocalReadCallback(handle *Handle, reader *ReadCallback, fn func() []byte) error {
	if err := checkReadMutation(); err != nil {
		return err
	}
	if handle == nil {
		return ErrClosed
	}
	handle.RLock()
	defer handle.RUnlock()
	if handle.NativeHandle == 0 {
		return ErrClosed
	}
	reader.Lock()
	defer reader.Unlock()
	var token, trampoline uintptr
	if fn != nil {
		token = uintptr(nextCallback.Add(1))
		trampoline = localReadTrampoline
		readRegistry.Lock()
		readRegistry.entries[token] = &readEntry{fn: fn}
		readRegistry.Unlock()
	}
	handle.Native.LocalCharacteristicOnRead(handle.NativeHandle, trampoline, token)
	reader.remove()
	if fn != nil {
		reader.owner, reader.token = handle, token
	}
	return nil
}
