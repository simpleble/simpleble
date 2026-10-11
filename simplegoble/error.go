package simplegoble

import "github.com/simpleble/simplegoble/internal/bindings"

// ErrClosed is returned when a wrapper is closed or has not been initialized.
var ErrClosed = bindings.ErrClosed

// ErrorCode matches the SimpleCBLE error classification. It can be used with
// errors.Is; use errors.As to inspect an Error's native diagnostic message.
type ErrorCode = bindings.ErrorCode

const (
	InvalidArgument            = bindings.InvalidArgument
	OutOfMemory                = bindings.OutOfMemory
	ObjectNotInitialized       = bindings.ObjectNotInitialized
	InvalidBackendReference    = bindings.InvalidBackendReference
	PeripheralNotConnected     = bindings.PeripheralNotConnected
	GATTServiceNotFound        = bindings.GATTServiceNotFound
	GATTCharacteristicNotFound = bindings.GATTCharacteristicNotFound
	GATTDescriptorNotFound     = bindings.GATTDescriptorNotFound
	OperationNotSupported      = bindings.OperationNotSupported
	OperationFailed            = bindings.OperationFailed
	WinRTAccessDenied          = bindings.WinRTAccessDenied
	WinRTException             = bindings.WinRTException
	CoreBluetoothException     = bindings.CoreBluetoothException
	UnclassifiedException      = bindings.UnclassifiedException
)

// Error is a native SimpleBLE failure. Message is copied before releasing the
// native error, so it remains valid after subsequent calls and Close.
type Error = bindings.Error
