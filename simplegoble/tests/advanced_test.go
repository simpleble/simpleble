package simplegoble_test

import (
	"errors"
	"testing"
	"unsafe"

	"github.com/ebitengine/purego"
	ble "github.com/simpleble/simplegoble"
	"github.com/simpleble/simplegoble/internal/bindings"
)

// requestPasskey calls a passkey request callback the same way SimpleCBLE does, with an empty char[7] buffer.
func (callback nativeCallback) requestPasskey() (bool, string) {
	buffer := make([]byte, 7)
	accepted, _, _ := purego.SyscallN(callback.trampoline, 1, uintptr(unsafe.Pointer(&buffer[0])), callback.token)
	return accepted&0xff != 0, string(buffer[:6])
}

// NOTE: Pairing can't be triggered without a Dongl and a device that asks for a passkey,
// so this test calls the passkey callback directly.
func TestPasskeyRequestCallback(t *testing.T) {
	var slot nativeCallback
	native := &bindings.NativeAPI{
		PeripheralRelease: func(uintptr) {},
		DonglPasskeyRequest: func(_ uintptr, trampoline, token uintptr, _ *uintptr) {
			slot = nativeCallback{trampoline, token}
		},
	}
	handle := &bindings.Handle{Native: native, NativeHandle: 1}
	owner := &bindings.CallbackOwner{}
	defer bindings.ClosePeripheral(handle, owner)

	for _, answer := range []struct {
		passkey  string
		ok       bool
		accepted bool
	}{
		{"012345", true, true},
		{"012345", false, false},
		{"12345", true, false},
		{"1234567", true, false},
	} {
		if err := bindings.SetPasskeyRequestCallback(handle, owner, func() (string, bool) { return answer.passkey, answer.ok }); err != nil {
			t.Fatal(err)
		}
		accepted, passkey := slot.requestPasskey()
		if accepted != answer.accepted || (accepted && passkey != answer.passkey) {
			t.Fatalf("answer %q, %v: accepted %v, passkey %q", answer.passkey, answer.ok, accepted, passkey)
		}
	}

	// A panic in the callback rejects the request instead of crashing.
	if err := bindings.SetPasskeyRequestCallback(handle, owner, func() (string, bool) { panic("user code") }); err != nil {
		t.Fatal(err)
	}
	if accepted, _ := slot.requestPasskey(); accepted {
		t.Fatal("panicking callback accepted the request")
	}
}

func TestPlainDonglPairingIsUnsupported(t *testing.T) {
	a := plainAdapter(t)
	p := plainPeripheral(t, a)
	if err := p.SetCallbackOnPasskeyRequest(func() (string, bool) { return "123456", true }); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
	if err := p.SetCallbackOnPasskeyDisplay(func(string) {}); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
	if err := p.SetCallbackOnNumericComparison(func(string) bool { return true }); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
	if err := p.SecureConnection(false); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
}

func TestPlainPlatformExtensionsAreUnsupported(t *testing.T) {
	a, host, _, _ := plainHost(t)
	if err := host.SetAdvertisementLocalName("SimpleGoBLE"); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
	if _, err := a.RetrieveCachedPeripherals([]string{"11:22:33:44:55:66"}); !errors.Is(err, ble.OperationNotSupported) {
		t.Fatal(err)
	}
}
