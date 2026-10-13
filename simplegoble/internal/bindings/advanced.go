package bindings

import (
	"unsafe"

	"github.com/ebitengine/purego"
)

type pairingReply struct {
	passkey string
	accept  bool
}

// SimpleBLE's pairing thread waits for these answers, so the handlers run on that
// thread instead of on the callback queue. A missing or panicking handler rejects
// the request.
var passkeyRequestTrampoline = purego.NewCallback(func(_ uintptr, passkey *byte, token uintptr) uintptr {
	var reply pairingReply
	if !answerPairing(token, CallbackPayload{reply: &reply}) || !reply.accept {
		return 0
	}
	// The C buffer only has room for six characters, and SimpleBLE validates the digits.
	if len(reply.passkey) != 6 {
		return 0
	}
	buffer := unsafe.Slice(passkey, 7)
	copy(buffer, reply.passkey)
	buffer[6] = 0
	return 1
})

var numericComparisonTrampoline = purego.NewCallback(func(_ uintptr, passkey *byte, token uintptr) uintptr {
	var reply pairingReply
	if !answerPairing(token, CallbackPayload{Text: CopyString(passkey), reply: &reply}) || !reply.accept {
		return 0
	}
	return 1
})

// NOTE: A panic must not unwind into the native pairing thread. This also skips the
// owner's registration lock: registering a callback holds it while waiting for the
// native slot, which the pairing thread holds during this call.
func answerPairing(token uintptr, payload CallbackPayload) (answered bool) {
	callbackRegistry.RLock()
	entry, ok := callbackRegistry.entries[token]
	callbackRegistry.RUnlock()
	if !ok {
		return false
	}
	entry.owner.mu.Lock()
	closed := entry.owner.closed
	entry.owner.mu.Unlock()
	if closed {
		return false
	}
	defer func() {
		if recover() != nil {
			answered = false
		}
	}()
	entry.fn(payload)
	return true
}

func SetPasskeyRequestCallback(handle *Handle, callbacks *CallbackOwner, fn func() (string, bool)) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { payload.reply.passkey, payload.reply.accept = fn() }
	}
	return setCallback(handle, callbacks, callbackKey{kind: passkeyRequest}, invoke, passkeyRequestTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, nativeError *uintptr) {
			native.DonglPasskeyRequest(h, trampoline, token, nativeError)
		})
}

// Display events don't need an answer, so they go through the callback queue.
func SetPasskeyDisplayCallback(handle *Handle, callbacks *CallbackOwner, fn func(string)) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { fn(payload.Text) }
	}
	return setCallback(handle, callbacks, callbackKey{kind: passkeyDisplay}, invoke, textTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, nativeError *uintptr) {
			native.DonglPasskeyDisplay(h, trampoline, token, nativeError)
		})
}

func SetNumericComparisonCallback(handle *Handle, callbacks *CallbackOwner, fn func(string) bool) error {
	var invoke func(CallbackPayload)
	if fn != nil {
		invoke = func(payload CallbackPayload) { payload.reply.accept = fn(payload.Text) }
	}
	return setCallback(handle, callbacks, callbackKey{kind: numericComparison}, invoke, numericComparisonTrampoline,
		func(native *NativeAPI, h, trampoline, token uintptr, nativeError *uintptr) {
			native.DonglNumericComparison(h, trampoline, token, nativeError)
		})
}

// SetAdvertisementLocalName sets the advertised name, or clears it if name is nil.
func SetAdvertisementLocalName(handle *Handle, name *string) error {
	if handle == nil {
		return ErrClosed
	}
	setName := handle.Native.SetAdvertisementLocalName
	if setName == nil {
		return &Error{Code: OperationNotSupported, Message: "advertisement local names require the macOS or Linux backend"}
	}
	var value *byte
	if name != nil {
		var err error
		if value, err = CString(*name); err != nil {
			return err
		}
	}
	return Command(handle, func(_ *NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		setName(nativeHandle, value, nativeError)
	})
}

// RetrieveCachedPeripherals returns owned handles for the known identifiers, in order.
func RetrieveCachedPeripherals(handle *Handle, identifiers []string) ([]uintptr, error) {
	if handle == nil {
		return nil, ErrClosed
	}
	retrieve := handle.Native.RetrieveCachedPeripheral
	if retrieve == nil {
		return nil, &Error{Code: OperationNotSupported, Message: "cached peripheral retrieval requires the macOS backend"}
	}
	values := make([]*byte, len(identifiers))
	for i, identifier := range identifiers {
		var err error
		if values[i], err = CString(identifier); err != nil {
			return nil, err
		}
	}
	return Query(handle, func(native *NativeAPI, nativeHandle uintptr, nativeError *uintptr) []uintptr {
		peripherals := make([]uintptr, 0, len(values))
		for _, value := range values {
			peripheral := retrieve(nativeHandle, value, nativeError)
			if *nativeError != 0 {
				for _, p := range peripherals {
					native.PeripheralRelease(p)
				}
				return nil
			}
			if peripheral != 0 {
				peripherals = append(peripherals, peripheral)
			}
		}
		return peripherals
	})
}
