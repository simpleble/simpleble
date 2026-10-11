package simplegoble_test

import (
	"bytes"
	"errors"
	"testing"

	"github.com/ebitengine/purego"
	"github.com/simpleble/simplegoble/internal/bindings"
)

type nativeCallback struct {
	trampoline uintptr
	token      uintptr
}

func TestSubscriptionInstallationAndFailure(t *testing.T) {
	service, _ := bindings.NewUUID("fff0")
	characteristic, _ := bindings.NewUUID("fff1")
	var current nativeCallback
	dispatch := func(callback nativeCallback, data []byte) {
		var invoke func(uintptr, bindings.UUID, bindings.UUID, *byte, uintptr, uintptr) uintptr
		purego.RegisterFunc(&invoke, callback.trampoline)
		invoke(1, service, characteristic, bindings.InputBytes(data), uintptr(len(data)), callback.token)
	}
	fail := false
	native := &bindings.NativeAPI{
		PeripheralRelease: func(uintptr) {},
		ErrorCode:         func(uintptr) uint32 { return uint32(bindings.OperationFailed) },
		ErrorMessage:      func(uintptr) string { return "subscription rejected" },
		ErrorRelease:      func(err *uintptr) { *err = 0 },
	}
	native.PeripheralNotify = func(_ uintptr, _, _ bindings.UUID, trampoline, token uintptr, err *uintptr) {
		callback := nativeCallback{trampoline, token}
		// Native installation may invoke the new callback synchronously. The
		// worker must wait for the registration outcome before dispatching it.
		data := []byte{0, 0xff}
		dispatch(callback, data)
		data[0] = 42 // Go must have copied native memory before returning.
		if fail {
			*err = 1
		} else {
			current = callback
		}
	}
	native.PeripheralUnsubscribe = func(_ uintptr, _, _ bindings.UUID, err *uintptr) {
		if fail {
			*err = 1
		}
	}
	var barrier nativeCallback
	native.PeripheralOnConnected = func(_ uintptr, trampoline, token uintptr) {
		barrier = nativeCallback{trampoline, token}
	}
	handle := &bindings.Handle{Native: native, NativeHandle: 1}
	owner := &bindings.CallbackOwner{}
	defer bindings.ClosePeripheral(handle, owner)
	values := make(chan []byte, 8)
	if err := bindings.SetDataCallback(handle, owner, service, characteristic, func(data []byte) { values <- data }, false); err != nil {
		t.Fatal(err)
	}
	if data := receive(t, values); !bytes.Equal(data, []byte{0, 0xff}) {
		t.Fatalf("native callback memory was retained: %v", data)
	}
	fail = true
	if err := bindings.SetDataCallback(handle, owner, service, characteristic, func([]byte) { values <- []byte{99} }, false); !errors.Is(err, bindings.OperationFailed) {
		t.Fatal(err)
	}
	if err := bindings.SetDataCallback(handle, owner, service, characteristic, nil, false); !errors.Is(err, bindings.OperationFailed) {
		t.Fatal(err)
	}
	dispatch(current, []byte{7})
	if data := receive(t, values); !bytes.Equal(data, []byte{7}) {
		t.Fatalf("failed installation replaced the old callback: %v", data)
	}
	fail = false
	if err := bindings.SetDataCallback(handle, owner, service, characteristic, nil, false); err != nil {
		t.Fatal(err)
	}
	dispatch(current, []byte{8}) // Late event from an unsubscribed native slot.
	done := make(chan struct{})
	if err := bindings.SetCallback(handle, owner, bindings.Connected, func() { close(done) }); err != nil {
		t.Fatal(err)
	}
	barrier.dispatch()
	receive(t, done)
	select {
	case data := <-values:
		t.Fatalf("expired callback ran: %v", data)
	default:
	}
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
