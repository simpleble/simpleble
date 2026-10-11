package simplegoble

import (
	"unsafe"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// LocalCharacteristic is an owned local GATT characteristic handle.
type LocalCharacteristic struct {
	handle    *bindings.Handle
	callbacks *bindings.CallbackOwner
	reader    *bindings.ReadCallback
}

func newLocalCharacteristic(native *bindings.NativeAPI, nativeHandle uintptr, reader *bindings.ReadCallback) *LocalCharacteristic {
	return &LocalCharacteristic{handle: &bindings.Handle{Native: native, NativeHandle: nativeHandle},
		callbacks: &bindings.CallbackOwner{}, reader: reader}
}

// Initialized reports whether the handle is open.
func (c *LocalCharacteristic) Initialized() bool { return c.handle.Initialized() }

// Close expires this wrapper's callbacks and releases its handle. A read callback
// still owned by this wrapper is cleared; a replacement on another handle is kept.
// Close does not remove the characteristic from its service.
func (c *LocalCharacteristic) Close() error {
	return bindings.CloseLocalCharacteristic(c.handle, c.callbacks, c.reader)
}

// UUID returns the normalized characteristic UUID.
func (c *LocalCharacteristic) UUID() (string, error) {
	return bindings.Query(c.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		var uuid bindings.UUID
		native.LocalCharacteristicUUID(nativeHandle, &uuid, nativeError)
		return uuid.String()
	})
}

// Capabilities returns the characteristic's capability flags.
func (c *LocalCharacteristic) Capabilities() (CharacteristicCapability, error) {
	return bindings.Query(c.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) CharacteristicCapability {
		return CharacteristicCapability(native.LocalCharacteristicCapabilities(nativeHandle, nativeError))
	})
}

// Value returns a Go-owned copy of the stored value, without invoking the read callback.
func (c *LocalCharacteristic) Value() ([]byte, error) {
	return bindings.Query(c.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []byte {
		var length uintptr
		data := native.LocalCharacteristicValue(nativeHandle, &length, nativeError)
		defer native.Free(unsafe.Pointer(data))
		return bindings.CopyBytes(data, length)
	})
}

// SetValue updates the stored value and publishes to subscribers on a best-effort basis.
// Keep the input slice unchanged until SetValue returns. Empty values are supported.
func (c *LocalCharacteristic) SetValue(data []byte) error {
	return bindings.Command(c.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.LocalCharacteristicSetValue(nativeHandle, bindings.InputBytes(data), uintptr(len(data)), nativeError)
	})
}

// SetCallbackOnRead supplies a dynamic read value; nil restores the stored value.
// Unlike event handlers, it runs synchronously on the native calling thread.
// Return promptly and do not call BLE methods, change registrations, or Close
// from this callback. The returned bytes are copied by Go and can contain zeros.
func (c *LocalCharacteristic) SetCallbackOnRead(fn func() []byte) error {
	return bindings.SetLocalReadCallback(c.handle, c.reader, fn)
}

// SetCallbackOnWrite observes incoming writes; nil clears it. Data is Go-owned.
// The native stored value is updated first. Handlers run serially on this wrapper's
// Go worker and may call characteristic methods, including SetValue or Close.
func (c *LocalCharacteristic) SetCallbackOnWrite(fn func([]byte)) error {
	return bindings.SetLocalWriteCallback(c.handle, c.callbacks, fn)
}

// SetCallbackOnSubscribed observes the first subscribed client; nil clears it.
// Dispatch follows SetCallbackOnWrite.
func (c *LocalCharacteristic) SetCallbackOnSubscribed(fn func()) error {
	return bindings.SetCallback(c.handle, c.callbacks, bindings.LocalSubscribed, fn)
}

// SetCallbackOnUnsubscribed observes the last client unsubscribing; nil clears it.
// Dispatch follows SetCallbackOnWrite.
func (c *LocalCharacteristic) SetCallbackOnUnsubscribed(fn func()) error {
	return bindings.SetCallback(c.handle, c.callbacks, bindings.LocalUnsubscribed, fn)
}
