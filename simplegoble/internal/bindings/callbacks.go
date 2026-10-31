package bindings

import (
	"runtime"
	"sync"
	"sync/atomic"

	"github.com/ebitengine/purego"
)

const (
	PowerOn = iota
	PowerOff
	ScanStart
	ScanStop
	Connected
	Disconnected
	ScanFound
	ScanUpdated
	subscription
	LocalConnected
	LocalDisconnected
	LocalSubscribed
	LocalUnsubscribed
	localWrite
	passkeyRequest
	passkeyDisplay
	numericComparison
	pairingAuthorization
)

type callbackKey struct {
	kind                    int
	service, characteristic string
}

// CallbackPayload has Go-owned data; Peripheral transfers an owned C handle.
// Signal callbacks leave all fields empty, and pairing handlers write their answer
// to reply. Linux pairing callbacks receive the peer's address in Peer.
type CallbackPayload struct {
	Peripheral uintptr
	Data       []byte
	Text       string
	Peer       string
	reply      *pairingReply
}

type callbackEntry struct {
	owner  *CallbackOwner
	key    callbackKey
	token  uintptr
	fn     func(CallbackPayload)
	handle *Handle // Keeps a wrapper with registered callbacks reachable.
}

type callbackEvent struct {
	entry   callbackEntry
	payload CallbackPayload
	drop    func()
}

var callbackRegistry = struct {
	sync.RWMutex
	entries map[uintptr]callbackEntry
}{entries: make(map[uintptr]callbackEntry)}
var nextCallback atomic.Uint64

// One trampoline per C signature, never per handle/registration. Ignore borrowed
// native handles; late events may refer to a wrapper that has already closed.
var signalTrampoline = purego.NewCallback(func(_ uintptr, token uintptr) uintptr {
	dispatchCallback(token, CallbackPayload{}, nil)
	return 0
})

var peripheralTrampoline = purego.NewCallback(func(_ uintptr, peripheral, token uintptr) uintptr {
	native, err := API()
	if err != nil {
		panic(err) // A native callback can only run after successful library loading.
	}
	dispatchCallback(token, CallbackPayload{Peripheral: peripheral}, func() { native.PeripheralRelease(peripheral) })
	return 0
})

// Used by both local client addresses and Dongl passkeys, as they share the same C signature.
var textTrampoline = purego.NewCallback(func(_ uintptr, text *byte, token uintptr) uintptr {
	dispatchCallback(token, CallbackPayload{Text: CopyString(text)}, nil)
	return 0
})

var dataTrampoline = func() uintptr {
	receive := func(data *byte, length, token uintptr) uintptr {
		dispatchCallback(token, CallbackPayload{Data: CopyBytes(data, length)}, nil)
		return 0
	}
	if runtime.GOOS == "windows" {
		// The Windows ABI passes these 37-byte UUID structs indirectly. Purego's
		// Windows callback bridge accepts pointers, but not struct parameters.
		return purego.NewCallback(func(_ uintptr, _ uintptr, _ uintptr, data *byte, length, token uintptr) uintptr {
			return receive(data, length, token)
		})
	}
	return purego.NewCallback(func(_ uintptr, _ UUID, _ UUID, data *byte, length, token uintptr) uintptr {
		return receive(data, length, token)
	})
}()

func dispatchCallback(token uintptr, payload CallbackPayload, drop func()) {
	callbackRegistry.RLock()
	entry, ok := callbackRegistry.entries[token]
	callbackRegistry.RUnlock()
	if !ok {
		if drop != nil {
			drop()
		}
		return
	}

	o := entry.owner
	o.mu.Lock()
	if o.closed {
		o.mu.Unlock()
		if drop != nil {
			drop()
		}
		return
	}
	o.queue = append(o.queue, callbackEvent{entry, payload, drop})
	o.wakeWorker()
	o.mu.Unlock()
}

type CallbackOwner struct {
	// Orders native installation and queue selection without holding mu during
	// native calls: SimpleBLE invokes callbacks while holding its callback lock.
	register sync.Mutex
	mu       sync.Mutex
	tokens   map[callbackKey]uintptr
	queue    []callbackEvent
	wake     chan struct{}
	closed   bool
}

func (o *CallbackOwner) wakeWorker() {
	select {
	case o.wake <- struct{}{}:
	default:
	}
}

func (o *CallbackOwner) close() {
	o.mu.Lock()
	o.closed = true
	queue := o.queue
	o.queue = nil
	callbackRegistry.Lock()
	for _, token := range o.tokens {
		delete(callbackRegistry.entries, token)
	}
	callbackRegistry.Unlock()
	o.tokens = nil
	if o.wake != nil {
		o.wakeWorker()
	}
	o.mu.Unlock()
	for _, event := range queue {
		if event.drop != nil {
			event.drop()
		}
	}
}

// Native callbacks only enqueue; user code runs serially on a Go goroutine.
// Handlers may change registrations or Close. A selected handler can finish
// after Close; discarded scan events release their transferred C handles.
func (o *CallbackOwner) run() {
	for range o.wake {
		for {
			o.register.Lock()
			o.mu.Lock()
			if o.closed {
				o.mu.Unlock()
				o.register.Unlock()
				return
			}
			if len(o.queue) == 0 {
				o.mu.Unlock()
				o.register.Unlock()
				break
			}
			event := o.queue[0]
			o.queue[0] = callbackEvent{}
			o.queue = o.queue[1:]
			current := o.tokens[event.entry.key] == event.entry.token
			o.mu.Unlock()
			o.register.Unlock()
			if current {
				event.entry.fn(event.payload)
			} else if event.drop != nil {
				event.drop()
			}
		}
	}
}

func setCallback(handle *Handle, callbacks *CallbackOwner, key callbackKey, fn func(CallbackPayload), trampoline uintptr,
	install func(*NativeAPI, uintptr, uintptr, uintptr, *uintptr)) error {
	if handle == nil {
		return ErrClosed
	}
	handle.RLock()
	defer handle.RUnlock()
	if handle.NativeHandle == 0 {
		return ErrClosed
	}
	callbacks.register.Lock()
	defer callbacks.register.Unlock()
	var token uintptr
	if fn != nil {
		token = uintptr(nextCallback.Add(1))
		callbacks.mu.Lock()
		if callbacks.wake == nil {
			callbacks.wake = make(chan struct{}, 1)
			go callbacks.run()
		}
		callbacks.mu.Unlock()
		callbackRegistry.Lock()
		callbackRegistry.entries[token] = callbackEntry{callbacks, key, token, fn, handle}
		callbackRegistry.Unlock()
	} else {
		trampoline = 0
	}
	var nativeError uintptr
	install(handle.Native, handle.NativeHandle, trampoline, token, &nativeError)
	if err := handle.Native.TakeError(&nativeError); err != nil {
		callbackRegistry.Lock()
		delete(callbackRegistry.entries, token)
		callbackRegistry.Unlock()
		return err
	}
	callbacks.mu.Lock()
	callbackRegistry.Lock()
	delete(callbackRegistry.entries, callbacks.tokens[key])
	callbackRegistry.Unlock()
	if callbacks.tokens == nil {
		callbacks.tokens = make(map[callbackKey]uintptr)
	}
	if token == 0 {
		delete(callbacks.tokens, key)
	} else {
		callbacks.tokens[key] = token
	}
	callbacks.mu.Unlock()
	return nil
}

func SetCallback(handle *Handle, callbacks *CallbackOwner, kind int, fn func()) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(CallbackPayload) { fn() }
	}
	return setCallback(handle, callbacks, callbackKey{kind: kind}, invoke, signalTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, _ *uintptr) {
			switch kind {
			case Connected:
				native.PeripheralOnConnected(h, trampoline, token)
			case Disconnected:
				native.PeripheralOnDisconnected(h, trampoline, token)
			case LocalSubscribed:
				native.LocalCharacteristicOnSubscribed(h, trampoline, token)
			case LocalUnsubscribed:
				native.LocalCharacteristicOnUnsubscribed(h, trampoline, token)
			default:
				native.SetCallbacks[kind](h, trampoline, token)
			}
		})
}

func SetPeripheralCallback(handle *Handle, callbacks *CallbackOwner, kind int, fn func(uintptr)) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { fn(payload.Peripheral) }
	}
	return setCallback(handle, callbacks, callbackKey{kind: kind}, invoke, peripheralTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, _ *uintptr) {
			if kind == ScanFound {
				native.AdapterScanFound(h, trampoline, token)
			} else {
				native.AdapterScanUpdated(h, trampoline, token)
			}
		})
}

func SetDataCallback(handle *Handle, callbacks *CallbackOwner, service, characteristic UUID, fn func([]byte), indicate bool) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { fn(payload.Data) }
	}
	key := callbackKey{subscription, service.String(), characteristic.String()}
	return setCallback(handle, callbacks, key, invoke, dataTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, nativeError *uintptr) {
			if fn == nil {
				native.PeripheralUnsubscribe(h, service, characteristic, nativeError)
			} else if indicate {
				native.PeripheralIndicate(h, service, characteristic, trampoline, token, nativeError)
			} else {
				native.PeripheralNotify(h, service, characteristic, trampoline, token, nativeError)
			}
		})
}
