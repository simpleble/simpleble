package simplegoble

import "github.com/simpleble/simplegoble/internal/bindings"

// SetCallbackOnPasskeyRequest sets the callback used when a Dongl needs a
// passkey to pair with this peripheral. Passing nil clears it.
//
// The callback returns the six digits typed by the user (including leading
// zeroes) and true, or false to reject the request. Register it before calling
// Connect, as pairing can start as soon as the connection is up.
//
// NOTE: The callback runs on SimpleBLE's pairing thread, which waits for the
// answer. It can block while asking the user, but it must not call other BLE
// methods, including Close. Peripherals from other backends return
// OperationNotSupported.
func (p *Peripheral) SetCallbackOnPasskeyRequest(fn func() (passkey string, ok bool)) error {
	return bindings.SetPasskeyRequestCallback(p.handle, p.callbacks, fn)
}

// SetCallbackOnNumericComparison sets the callback used when a Dongl asks to
// confirm the number shown by this peripheral. Passing nil clears it.
//
// The callback receives the six digits and returns true only if the user
// confirms that the device shows the same number. It runs the same way as the
// SetCallbackOnPasskeyRequest callback.
func (p *Peripheral) SetCallbackOnNumericComparison(fn func(passkey string) bool) error {
	return bindings.SetNumericComparisonCallback(p.handle, p.callbacks, fn)
}

// SetCallbackOnPasskeyDisplay sets the callback that receives the six-digit
// passkey the user must type on the device. Passing nil clears it. Register it
// before calling Connect.
//
// NOTE: This callback runs like the one from SetCallbackOnConnected.
// Peripherals from other backends return OperationNotSupported.
func (p *Peripheral) SetCallbackOnPasskeyDisplay(fn func(passkey string)) error {
	return bindings.SetPasskeyDisplayCallback(p.handle, p.callbacks, fn)
}

// SecureConnection secures the connection with LE Secure Connections. With
// mitm, the peer must also be authenticated through the passkey or numeric
// comparison callbacks.
//
// NOTE: Pairing started by this call creates no bond, and the Dongl
// disconnects if it fails. Peripherals from other backends return
// OperationNotSupported.
func (p *Peripheral) SecureConnection(mitm bool) error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.DonglSecureConnection(nativeHandle, mitm, nativeError)
	})
}

// SetAdvertisementLocalName sets the name included in the advertisement on
// macOS and Linux. Call it before Start.
//
// NOTE: This only changes the advertisement, not the system Bluetooth name.
// Other platforms return OperationNotSupported.
func (p *LocalPeripheral) SetAdvertisementLocalName(name string) error {
	return bindings.SetAdvertisementLocalName(p.handle, &name)
}

// ClearAdvertisementLocalName removes the name set by SetAdvertisementLocalName.
// Call it before Start.
func (p *LocalPeripheral) ClearAdvertisementLocalName() error {
	return bindings.SetAdvertisementLocalName(p.handle, nil)
}

// RetrieveCachedPeripherals returns the peripherals that CoreBluetooth already
// knows about on macOS, using the identifiers returned by Peripheral.Address.
// Unknown identifiers are skipped. Close every returned peripheral.
//
// NOTE: This doesn't scan, so a returned peripheral might be out of range.
// The adapter must be powered on. Other platforms return OperationNotSupported.
func (a *Adapter) RetrieveCachedPeripherals(identifiers []string) ([]*Peripheral, error) {
	handles, err := bindings.RetrieveCachedPeripherals(a.handle, identifiers)
	if err != nil {
		return nil, err
	}
	peripherals := make([]*Peripheral, 0, len(handles))
	for _, nativeHandle := range handles {
		peripherals = append(peripherals, newPeripheral(a.handle.Native, nativeHandle))
	}
	return peripherals, nil
}
