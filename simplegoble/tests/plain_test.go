package simplegoble_test

import (
	"errors"
	"runtime"
	"testing"
	"time"

	"github.com/simpleble/simplegoble"
	"github.com/simpleble/simplegoble/internal/bindings"
)

func plainAdapter(t *testing.T) *simplegoble.Adapter {
	t.Helper()
	path := plainLibrary(t)
	if err := bindings.Load(path); err != nil {
		t.Fatal(err)
	}
	backends, err := simplegoble.GetBackends()
	if err != nil {
		t.Fatal(err)
	}
	defer func() {
		for _, b := range backends {
			_ = b.Close()
		}
	}()
	if len(backends) != 1 {
		t.Fatalf("expected one PLAIN backend, got %d", len(backends))
	}
	adapters, err := backends[0].Adapters()
	if err != nil {
		t.Fatal(err)
	}
	for _, a := range adapters {
		t.Cleanup(func() { _ = a.Close() })
	}
	if len(adapters) != 1 {
		t.Fatalf("expected one PLAIN adapter, got %d", len(adapters))
	}
	a := adapters[0]
	if err := a.PowerOn(); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	// Drain setup events before a test installs its own callbacks. PLAIN looks
	// up callback slots when the asynchronous event executes.
	ready := make(chan struct{}, 1)
	// Several queued stop events may reach this temporary slot. Coalesce them
	// so a duplicate cannot block the worker after the setup wait finishes.
	if err := a.SetCallbackOnScanStop(func() {
		select {
		case ready <- struct{}{}:
		default:
		}
	}); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	receive(t, ready)
	if err := a.SetCallbackOnScanStop(nil); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = a.ScanStop(); _ = a.PowerOn() })
	return a
}

func receive[T any](t *testing.T, ch <-chan T) T {
	t.Helper()
	select {
	case value := <-ch:
		return value
	case <-time.After(5 * time.Second):
		t.Fatal("timed out waiting for native callback")
	}
	var zero T
	return zero
}

func TestPlainHandleOwnership(t *testing.T) {
	a := plainAdapter(t)
	backends, err := simplegoble.GetBackends()
	if err != nil {
		t.Fatal(err)
	}
	b := backends[0]
	copyB := *b
	if err := b.Close(); err != nil {
		t.Fatal(err)
	}
	if b.Initialized() || copyB.Initialized() {
		t.Fatal("backend copy retained a released handle")
	}
	if _, err := copyB.Adapters(); !errors.Is(err, simplegoble.ErrClosed) {
		t.Fatal(err)
	}
	if err := copyB.Close(); err != nil {
		t.Fatal(err)
	}
	adapters, err := simplegoble.GetAdapters()
	if err != nil {
		t.Fatal(err)
	}
	for _, other := range adapters {
		defer other.Close()
	}
	if len(adapters) != 1 {
		t.Fatalf("adapter count = %d", len(adapters))
	}
	if err := a.PowerOff(); err != nil {
		t.Fatal(err)
	}
	if powered, err := adapters[0].IsPowered(); err != nil || powered {
		t.Fatalf("shared power = %v, %v", powered, err)
	}
	if err := adapters[0].PowerOn(); err != nil {
		t.Fatal(err)
	}
	if powered, err := a.IsPowered(); err != nil || !powered {
		t.Fatalf("shared power = %v, %v", powered, err)
	}
	copyA := *adapters[0]
	if err := adapters[0].Close(); err != nil {
		t.Fatal(err)
	}
	if copyA.Initialized() {
		t.Fatal("adapter copy retained a released handle")
	}
	if _, err := copyA.Identifier(); !errors.Is(err, simplegoble.ErrClosed) {
		t.Fatal(err)
	}
	if err := copyA.SetCallbackOnScanStart(func() {}); !errors.Is(err, simplegoble.ErrClosed) {
		t.Fatal(err)
	}
	if err := copyA.Close(); err != nil {
		t.Fatal(err)
	}
	if !a.Initialized() {
		t.Fatal("closing an independent handle closed the original")
	}
}

func TestPlainNamedBackends(t *testing.T) {
	if err := bindings.Load(plainLibrary(t)); err != nil {
		t.Fatal(err)
	}
	native, err := simplegoble.GetNativeBackend()
	if err != nil {
		t.Fatal(err)
	}
	defer native.Close()
	if name, err := native.Identifier(); err != nil || name != "Plain" {
		t.Fatalf("native backend = %q, %v", name, err)
	}
	if _, err := simplegoble.GetDonglBackend(); !errors.Is(err, simplegoble.OperationNotSupported) {
		t.Fatalf("GetDonglBackend: %v", err)
	}
	if _, err := simplegoble.GetSimulatorBackend(); !errors.Is(err, simplegoble.OperationNotSupported) {
		t.Fatalf("GetSimulatorBackend: %v", err)
	}
}

func TestPlainRejectsInvalidScanDuration(t *testing.T) {
	a := plainAdapter(t)
	for _, d := range []time.Duration{-1, time.Duration(1<<31) * time.Millisecond} {
		if err := a.ScanFor(d); !errors.Is(err, simplegoble.InvalidArgument) {
			t.Fatal(err)
		}
	}

}

func TestPlainCallbacksAndReplacement(t *testing.T) {
	a := plainAdapter(t)
	events := make(chan string, 16)
	register := []struct {
		set  func(func()) error
		name string
	}{
		{a.SetCallbackOnPowerOff, "off"}, {a.SetCallbackOnPowerOn, "on"},
		{a.SetCallbackOnScanStart, "start"}, {a.SetCallbackOnScanStop, "stop"},
	}
	for _, r := range register {
		if err := r.set(func() {
			// Re-entering Go->native from a callback must not deadlock.
			if _, err := a.Identifier(); err != nil {
				events <- err.Error()
				return
			}
			events <- r.name
		}); err != nil {
			t.Fatal(err)
		}
	}
	for _, action := range []struct {
		call func() error
		want string
	}{
		{a.PowerOff, "off"}, {a.PowerOn, "on"}, {a.ScanStart, "start"}, {a.ScanStop, "stop"},
	} {
		if err := action.call(); err != nil {
			t.Fatal(err)
		}
		if event := receive(t, events); event != action.want {
			t.Fatalf("got %s, want %s", event, action.want)
		}
		if action.want == "start" || action.want == "stop" {
			active, err := a.ScanIsActive()
			if err != nil || active != (action.want == "start") {
				t.Fatalf("scan state = %v, %v", active, err)
			}
		}
	}
	if err := a.SetCallbackOnScanStart(func() { events <- "replacement" }); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStart(); err != nil {
		t.Fatal(err)
	}
	if event := receive(t, events); event != "replacement" {
		t.Fatal(event)
	}
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	if event := receive(t, events); event != "stop" {
		t.Fatal(event)
	}
	if err := a.SetCallbackOnScanStart(nil); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStart(); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	// The stop callback is a native queue barrier for the cleared start slot.
	if event := receive(t, events); event != "stop" {
		t.Fatal(event)
	}
	for _, r := range register {
		if err := r.set(nil); err != nil {
			t.Fatal(err)
		}
	}
}

func TestPlainCallbackCanClose(t *testing.T) {
	a := plainAdapter(t)
	done := make(chan error, 1)
	if err := a.SetCallbackOnScanStart(func() { done <- a.Close() }); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStart(); err != nil {
		t.Fatal(err)
	}
	if err := receive(t, done); err != nil {
		t.Fatal(err)
	}
	if a.Initialized() {
		t.Fatal("callback did not close adapter")
	}
	// Close preserves native scan state; clean up using an independent handle.
	b := plainAdapter(t)
	if err := b.ScanStop(); err != nil {
		t.Fatal(err)
	}
}

func TestPlainCloseDoesNotClearAnotherHandleCallback(t *testing.T) {
	a := plainAdapter(t)
	b := plainAdapter(t)
	done := make(chan struct{}, 1)
	if err := a.SetCallbackOnScanStart(func() { t.Error("replaced callback ran") }); err != nil {
		t.Fatal(err)
	}
	if err := b.SetCallbackOnScanStart(func() { done <- struct{}{} }); err != nil {
		t.Fatal(err)
	}
	if err := a.Close(); err != nil {
		t.Fatal(err)
	}
	if err := b.ScanStart(); err != nil {
		t.Fatal(err)
	}
	receive(t, done)
	if err := b.ScanStop(); err != nil {
		t.Fatal(err)
	}
}

func TestPlainLateNativeCallbackAfterClose(t *testing.T) {
	a := plainAdapter(t)
	b := plainAdapter(t)
	called := make(chan struct{}, 1)
	if err := a.SetCallbackOnScanStart(func() { called <- struct{}{} }); err != nil {
		t.Fatal(err)
	}
	if err := a.Close(); err != nil {
		t.Fatal(err)
	}
	barrier := make(chan struct{}, 1)
	if err := b.SetCallbackOnScanStop(func() { barrier <- struct{}{} }); err != nil {
		t.Fatal(err)
	}
	// The start slot still contains a C lambda capturing a's released handle.
	// Dispatch must ignore both that opaque handle value and its expired token.
	if err := b.ScanStart(); err != nil {
		t.Fatal(err)
	}
	if err := b.ScanStop(); err != nil {
		t.Fatal(err)
	}
	receive(t, barrier)
	select {
	case <-called:
		t.Fatal("closed handle's native callback reached Go user code")
	default:
	}
}

func TestPlainRepeatedRegistrationAndDisposal(t *testing.T) {
	_ = plainAdapter(t)
	// Exceeds purego's per-process callback limit if trampolines are allocated
	// per registration. Also exercises stale callbacks and native-handle reuse.
	for i := 0; i < 2100; i++ {
		adapters, err := simplegoble.GetAdapters()
		if err != nil {
			t.Fatal(err)
		}
		a := adapters[0]
		if err := a.SetCallbackOnScanStart(func() {}); err != nil {
			t.Fatal(err)
		}
		if err := a.Close(); err != nil {
			t.Fatal(err)
		}
	}
}

func TestPlainBlockingScanAndConcurrentClose(t *testing.T) {
	a := plainAdapter(t)
	previous := runtime.GOMAXPROCS(1)
	defer runtime.GOMAXPROCS(previous)
	started := make(chan struct{}, 1)
	if err := a.SetCallbackOnScanStart(func() { started <- struct{}{} }); err != nil {
		t.Fatal(err)
	}
	done := make(chan error, 1)
	go func() { done <- a.ScanFor(350 * time.Millisecond) }()
	receive(t, started)
	// With one P, a blocking FFI call must yield to unrelated goroutines.
	select {
	case err := <-done:
		t.Fatalf("scan finished before goroutines could run: %v", err)
	case <-time.After(20 * time.Millisecond):
	}
	// Other calls on the same wrapper must not wait for the scan.
	if _, err := a.IsPowered(); err != nil {
		t.Fatal(err)
	}
	select {
	case err := <-done:
		t.Fatalf("IsPowered waited for the scan: %v", err)
	default:
	}
	closed := make(chan error, 1)
	go func() { closed <- a.Close() }()
	select {
	case <-closed:
		t.Fatal("Close released a handle during a native call")
	case <-time.After(20 * time.Millisecond):
	}
	if err := receive(t, done); err != nil {
		t.Fatal(err)
	}
	if err := receive(t, closed); err != nil {
		t.Fatal(err)
	}
}
