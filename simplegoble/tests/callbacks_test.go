package simplegoble_test

import (
	"testing"

	"github.com/ebitengine/purego"
	"github.com/simpleble/simplegoble/internal/bindings"
)

type nativeCallback struct {
	trampoline uintptr
	token      uintptr
}

func (callback nativeCallback) dispatch() {
	purego.SyscallN(callback.trampoline, 0, callback.token)
}

func callbackAdapter() (*bindings.Handle, *bindings.CallbackOwner, *[4]nativeCallback) {
	var callbacks [4]nativeCallback
	native := &bindings.NativeAPI{AdapterRelease: func(uintptr) {}}
	for kind := range callbacks {
		native.SetCallbacks[kind] = func(_ uintptr, trampoline, token uintptr) {
			callbacks[kind] = nativeCallback{trampoline, token}
		}
	}
	return &bindings.Handle{Native: native, NativeHandle: 1}, &bindings.CallbackOwner{}, &callbacks
}

func TestReplacingCallbackDiscardsQueuedEvents(t *testing.T) {
	handle, owner, callbacks := callbackAdapter()
	defer bindings.CloseAdapter(handle, owner)
	entered := make(chan struct{})
	release := make(chan struct{})
	finished := make(chan struct{})
	if err := bindings.SetCallback(handle, owner, bindings.ScanStart, func() { close(entered); <-release; close(finished) }); err != nil {
		t.Fatal(err)
	}
	callbacks[bindings.ScanStart].dispatch()
	receive(t, entered)
	called := make(chan string, 3)
	if err := bindings.SetCallback(handle, owner, bindings.ScanStop, func() { called <- "old" }); err != nil {
		t.Fatal(err)
	}
	old := callbacks[bindings.ScanStop]
	old.dispatch() // Queued behind the running start handler.
	if err := bindings.SetCallback(handle, owner, bindings.ScanStop, func() { called <- "current" }); err != nil {
		t.Fatal(err)
	}
	old.dispatch() // An expired token must never target its replacement.
	callbacks[bindings.ScanStop].dispatch()
	close(release)
	receive(t, finished)
	if event := receive(t, called); event != "current" {
		t.Fatalf("replaced callback ran: %s", event)
	}
}
