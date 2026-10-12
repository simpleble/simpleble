package bindings

import (
	"errors"
	"fmt"
)

// ErrClosed is returned when a wrapper is closed or has not been initialized.
var ErrClosed = errors.New("simplegoble: closed or uninitialized handle")

// ErrorCode matches the SimpleCBLE error classification. It can be used with
// errors.Is; use errors.As to inspect an Error's native diagnostic message.
type ErrorCode uint32

const (
	InvalidArgument ErrorCode = iota
	OutOfMemory
	ObjectNotInitialized
	InvalidBackendReference
	PeripheralNotConnected
	GATTServiceNotFound
	GATTCharacteristicNotFound
	GATTDescriptorNotFound
	OperationNotSupported
	OperationFailed
	WinRTAccessDenied
	WinRTException
	CoreBluetoothException
	UnclassifiedException
	AdapterInUse
)

func (c ErrorCode) Error() string {
	names := [...]string{"invalid argument", "out of memory", "object not initialized",
		"invalid backend reference", "peripheral not connected", "GATT service not found",
		"GATT characteristic not found", "GATT descriptor not found", "operation not supported",
		"operation failed", "WinRT access denied", "WinRT exception", "CoreBluetooth exception",
		"unclassified exception", "adapter in use"}
	if uint32(c) < uint32(len(names)) {
		return "simplegoble: " + names[c]
	}
	return fmt.Sprintf("simplegoble: error code %d", c)
}

// Error is a native SimpleBLE failure. Message is copied before releasing the
// native error, so it remains valid after subsequent calls and Close.
type Error struct {
	Code    ErrorCode
	Message string
}

func (e *Error) Error() string { return fmt.Sprintf("%s: %s", e.Code, e.Message) }
func (e *Error) Is(target error) bool {
	c, ok := target.(ErrorCode)
	return ok && c == e.Code
}
