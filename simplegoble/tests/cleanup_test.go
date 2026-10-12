package simplegoble_test

import (
	"runtime"
	"sync/atomic"
	"testing"
	"time"
	"weak"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// releasedAfterGC collects until want releases happened or a deadline passes,
// then keeps collecting briefly so an extra release would surface.
func releasedAfterGC(released *atomic.Int32, want int32) int32 {
	for deadline := time.Now().Add(2 * time.Second); released.Load() < want && time.Now().Before(deadline); {
		runtime.GC()
		time.Sleep(5 * time.Millisecond)
	}
	for range 5 {
		runtime.GC()
		time.Sleep(5 * time.Millisecond)
	}
	return released.Load()
}

func TestUnreachableHandlesAreReleasedOnce(t *testing.T) {
	var released atomic.Int32
	native := &bindings.NativeAPI{PeripheralRelease: func(uintptr) { released.Add(1) }}
	func() {
		owner := &bindings.CallbackOwner{}
		if err := bindings.ClosePeripheral(bindings.NewPeripheral(native, 1, owner), owner); err != nil {
			t.Fatal(err)
		}
		bindings.NewPeripheral(native, 2, &bindings.CallbackOwner{})
	}()
	// One release from Close and one from the unreachable handle; Close stops its cleanup.
	if n := releasedAfterGC(&released, 2); n != 2 {
		t.Fatalf("released %d times, want 2", n)
	}
}

func TestRegisteredCallbacksKeepHandlesReachable(t *testing.T) {
	var released atomic.Int32
	native := &bindings.NativeAPI{AdapterRelease: func(uintptr) { released.Add(1) }}
	native.SetCallbacks[bindings.ScanStart] = func(uintptr, uintptr, uintptr) {}
	owner := &bindings.CallbackOwner{}
	pointer := func() weak.Pointer[bindings.Handle] {
		handle := bindings.NewAdapter(native, 1, owner)
		if err := bindings.SetCallback(handle, owner, bindings.ScanStart, func() {}); err != nil {
			t.Fatal(err)
		}
		return weak.Make(handle)
	}()
	if n := releasedAfterGC(&released, 0); n != 0 {
		t.Fatal("released a handle with a registered callback")
	}
	func() {
		handle := pointer.Value()
		if handle == nil {
			t.Fatal("collected a handle with a registered callback")
		}
		if err := bindings.SetCallback(handle, owner, bindings.ScanStart, nil); err != nil {
			t.Fatal(err)
		}
	}()
	if n := releasedAfterGC(&released, 1); n != 1 {
		t.Fatalf("released %d times after clearing the callback, want 1", n)
	}
}
