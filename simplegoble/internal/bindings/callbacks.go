package bindings

import (
	"sync"
	"sync/atomic"

	"github.com/ebitengine/purego"
)

const (
	PowerOn = iota
	PowerOff
	ScanStart
	ScanStop
)

type callbackEntry struct {
	owner *CallbackOwner
	kind  int
	token uintptr
	fn    func()
}

var callbackRegistry = struct {
	sync.RWMutex
	entries map[uintptr]callbackEntry
}{entries: make(map[uintptr]callbackEntry)}
var nextCallback atomic.Uint64

// Exactly one trampoline for the process, never one per handle/registration.
// userdata is a monotonically increasing integer, never a Go pointer. Ignore
// the native adapter argument: a late callback may carry a released C handle.
var signalTrampoline = purego.NewCallback(func(_ uintptr, token uintptr) uintptr {
	dispatchCallback(token)
	return 0
})

func dispatchCallback(token uintptr) {
	callbackRegistry.RLock()
	entry, ok := callbackRegistry.entries[token]
	callbackRegistry.RUnlock()
	if !ok {
		return
	}
	o := entry.owner
	o.mu.Lock()
	defer o.mu.Unlock()
	if o.closed || o.tokens[entry.kind] != token {
		return
	}
	o.queue = append(o.queue, entry)
	o.wakeWorker()
}

type CallbackOwner struct {
	// Orders token changes with native installation. Kept separate from mu:
	// native dispatch takes mu while SimpleBLE holds its callback lock.
	register sync.Mutex
	mu       sync.Mutex
	tokens   [4]uintptr
	queue    []callbackEntry
	wake     chan struct{}
	closed   bool
}

func (o *CallbackOwner) set(kind int, fn func()) uintptr {
	o.mu.Lock()
	defer o.mu.Unlock()
	callbackRegistry.Lock()
	defer callbackRegistry.Unlock()
	delete(callbackRegistry.entries, o.tokens[kind])
	o.tokens[kind] = 0
	if fn == nil {
		return 0
	}
	token := uintptr(nextCallback.Add(1))
	o.tokens[kind] = token
	callbackRegistry.entries[token] = callbackEntry{owner: o, kind: kind, token: token, fn: fn}
	if o.wake == nil {
		o.wake = make(chan struct{}, 1)
		go o.run()
	}
	return token
}

func (o *CallbackOwner) wakeWorker() {
	select {
	case o.wake <- struct{}{}:
	default:
	}
}

func (o *CallbackOwner) close() {
	o.mu.Lock()
	defer o.mu.Unlock()
	o.closed = true
	o.queue = nil
	callbackRegistry.Lock()
	for _, token := range o.tokens {
		delete(callbackRegistry.entries, token)
	}
	callbackRegistry.Unlock()
	o.tokens = [4]uintptr{}
	if o.wake != nil {
		o.wakeWorker()
	}
}

// Native callbacks only enqueue; user code runs serially on a Go goroutine.
// This lets handlers call methods, change callbacks or Close without holding a
// native callback mutex. A handler already selected can finish after Close.
func (o *CallbackOwner) run() {
	for range o.wake {
		for {
			o.mu.Lock()
			if o.closed {
				o.mu.Unlock()
				return
			}
			if len(o.queue) == 0 {
				o.mu.Unlock()
				break
			}
			entry := o.queue[0]
			o.queue[0] = callbackEntry{}
			o.queue = o.queue[1:]
			current := o.tokens[entry.kind] == entry.token
			o.mu.Unlock()
			if current {
				entry.fn()
			}
		}
	}
}

func SetCallback(handle *Handle, callbacks *CallbackOwner, kind int, fn func()) error {
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
	token := callbacks.set(kind, fn)
	var trampoline uintptr
	if token != 0 {
		trampoline = signalTrampoline
	}
	handle.Native.SetCallbacks[kind](handle.NativeHandle, trampoline, token)
	return nil
}
