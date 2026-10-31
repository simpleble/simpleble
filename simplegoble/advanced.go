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

// SetCallbackOnPasskeyRequest sets the callback used on Linux when a peer
// pairing through this adapter needs a passkey. Passing nil clears it.
//
// The callback receives the peer's address and returns the six digits typed by
// the user (including leading zeroes) and true, or false to reject the request.
// It covers the peripherals this adapter connects to and the clients of its
// local peripherals. Requests only reach it when
// Config.SimpleBluez.SetInteractivePairing(true) was called before the first
// adapter call.
//
// NOTE: The callback runs on SimpleBLE's BlueZ event thread, which waits for the
// answer. It can block while asking the user, but it must not call other BLE
// methods, including Close. Native callback slots are shared across handles for
// the same adapter: the latest registration wins. Adapters from other backends,
// such as Dongl, and other platforms return OperationNotSupported.
func (a *Adapter) SetCallbackOnPasskeyRequest(fn func(peer string) (passkey string, ok bool)) error {
	return bindings.SetLinuxPasskeyRequestCallback(a.handle, a.callbacks, fn)
}

// SetCallbackOnNumericComparison sets the callback used on Linux when a peer
// pairing through this adapter asks to confirm a number. Passing nil clears it.
//
// The callback receives the peer's address and the six digits, and returns true
// only if the user confirms that the peer shows the same number. It runs the
// same way as the SetCallbackOnPasskeyRequest callback.
func (a *Adapter) SetCallbackOnNumericComparison(fn func(peer, passkey string) bool) error {
	return bindings.SetLinuxNumericComparisonCallback(a.handle, a.callbacks, fn)
}

// SetCallbackOnPasskeyDisplay sets the callback that receives, on Linux, the
// peer's address and the six-digit passkey the user must type on the peer.
// Passing nil clears it.
//
// NOTE: This callback runs like the one from SetCallbackOnPowerOn. Adapters
// from other backends and other platforms return OperationNotSupported.
func (a *Adapter) SetCallbackOnPasskeyDisplay(fn func(peer, passkey string)) error {
	return bindings.SetLinuxPasskeyDisplayCallback(a.handle, a.callbacks, fn)
}

// SetCallbackOnPairingAuthorization sets the callback that approves Just Works
// pairing with a peer through this adapter on Linux. Passing nil clears it, and
// without it Just Works pairing is accepted.
//
// The callback receives the peer's address and returns true to accept the
// pairing. It runs the same way as the SetCallbackOnPasskeyRequest callback.
// After Close, the callback rejects requests, so clear it instead to accept
// Just Works pairing again.
func (a *Adapter) SetCallbackOnPairingAuthorization(fn func(peer string) bool) error {
	return bindings.SetLinuxPairingAuthorizationCallback(a.handle, a.callbacks, fn)
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
