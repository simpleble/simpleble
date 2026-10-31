package simplegoble

import (
	"math"
	"time"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// Configuration groups the native settings of each backend.
//
// NOTE: Set these once at startup, before calling GetBackends, GetAdapters or
// BluetoothEnabled. Changes made after a backend has started might not take
// effect, and the settings are not safe to change from several goroutines.
type Configuration struct {
	SimpleBluez   SimpleBluezConfig
	WinRT         WinRTConfig
	CoreBluetooth CoreBluetoothConfig
	Android       AndroidConfig
	Dongl         DonglConfig
}

// Config is the process-wide native configuration, for example
// ble.Config.Dongl.SetUseDonglBackend(false).
var Config Configuration

// SimpleBluezConfig configures the Linux BlueZ backend.
type SimpleBluezConfig struct{}

// WinRTConfig configures the Windows backend.
type WinRTConfig struct{}

// CoreBluetoothConfig configures the macOS backend. It has no options yet.
type CoreBluetoothConfig struct{}

// AndroidConfig configures the Android backend.
type AndroidConfig struct{}

// DonglConfig configures the Dongl backend.
type DonglConfig struct{}

func configure(set func(*bindings.NativeAPI)) error {
	native, err := bindings.API()
	if err != nil {
		return err
	}
	set(native)
	return nil
}

func configValue[T any](get func(*bindings.NativeAPI) T) (T, error) {
	native, err := bindings.API()
	if err != nil {
		var zero T
		return zero, err
	}
	return get(native), nil
}

// Fractional milliseconds round up, like ScanFor.
// NOTE: SimpleBLE stores timeouts in nanoseconds, so rounding up the longest
// Durations would overflow there.
func timeoutMilliseconds(timeout time.Duration) (int64, error) {
	if timeout < 0 {
		return 0, &Error{Code: InvalidArgument, Message: "timeout cannot be negative"}
	}
	ms := timeout / time.Millisecond
	if timeout%time.Millisecond > 0 {
		ms++
	}
	if ms > math.MaxInt64/time.Millisecond {
		return 0, &Error{Code: InvalidArgument, Message: "timeout is too long"}
	}
	return int64(ms), nil
}

// ResetAll restores every backend's defaults.
func (Configuration) ResetAll() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigResetAll() })
}

// Reset restores the BlueZ defaults.
func (SimpleBluezConfig) Reset() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigBluezReset() })
}

// UseSystemBus reports whether BlueZ is reached over the D-Bus system bus
// rather than the session bus. Defaults to true.
func (SimpleBluezConfig) UseSystemBus() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigBluezSystemBus() })
}

// SetUseSystemBus selects the D-Bus system bus (true) or session bus (false).
func (SimpleBluezConfig) SetUseSystemBus(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigBluezSetSystemBus(enabled) })
}

// InteractivePairing reports whether SimpleBLE's BlueZ agent passes passkey and
// numeric comparison requests to the adapter pairing callbacks, such as
// Adapter.SetCallbackOnPasskeyRequest. Defaults to false, which allows only
// Just Works pairing.
func (SimpleBluezConfig) InteractivePairing() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigBluezInteractivePairing() })
}

// SetInteractivePairing sets whether passkey and numeric comparison requests
// reach the adapter pairing callbacks. SimpleBLE reads it once when the BlueZ
// backend starts, so set it before the first adapter call.
func (SimpleBluezConfig) SetInteractivePairing(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigBluezSetInteractivePairing(enabled) })
}

// ConnectionTimeout returns how long Connect waits for BlueZ. Defaults to 2 seconds.
func (SimpleBluezConfig) ConnectionTimeout() (time.Duration, error) {
	return configValue(func(native *bindings.NativeAPI) time.Duration {
		return time.Duration(native.ConfigBluezConnectTimeout()) * time.Millisecond
	})
}

// SetConnectionTimeout sets how long Connect waits for BlueZ. Fractional
// milliseconds round up, and negative timeouts return InvalidArgument.
func (SimpleBluezConfig) SetConnectionTimeout(timeout time.Duration) error {
	ms, err := timeoutMilliseconds(timeout)
	if err != nil {
		return err
	}
	return configure(func(native *bindings.NativeAPI) { native.ConfigBluezSetConnectTimeout(ms) })
}

// DisconnectionTimeout returns how long Disconnect waits for BlueZ. Defaults to 1 second.
func (SimpleBluezConfig) DisconnectionTimeout() (time.Duration, error) {
	return configValue(func(native *bindings.NativeAPI) time.Duration {
		return time.Duration(native.ConfigBluezDisconnectTimeout()) * time.Millisecond
	})
}

// SetDisconnectionTimeout sets how long Disconnect waits for BlueZ. Fractional
// milliseconds round up, and negative timeouts return InvalidArgument.
func (SimpleBluezConfig) SetDisconnectionTimeout(timeout time.Duration) error {
	ms, err := timeoutMilliseconds(timeout)
	if err != nil {
		return err
	}
	return configure(func(native *bindings.NativeAPI) { native.ConfigBluezSetDisconnectTimeout(ms) })
}

// Reset restores the Windows defaults.
func (WinRTConfig) Reset() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigWinRTReset() })
}

// ExperimentalUseOwnMTAApartment reports whether WinRT calls run on
// SimpleBLE's own MTA thread. Defaults to true.
//
// Deprecated: SimpleBLE uses its own WinRT MTA apartment by default. This
// compatibility flag will be removed in a future release.
func (WinRTConfig) ExperimentalUseOwnMTAApartment() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigWinRTOwnMTA() })
}

// SetExperimentalUseOwnMTAApartment sets whether WinRT calls run on
// SimpleBLE's own MTA thread.
//
// Deprecated: SimpleBLE uses its own WinRT MTA apartment by default. This
// compatibility flag will be removed in a future release.
func (WinRTConfig) SetExperimentalUseOwnMTAApartment(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigWinRTSetOwnMTA(enabled) })
}

// ExperimentalReinitializeWinRTApartmentOnMainThread reports whether the
// backend reinitializes the WinRT apartment on the calling thread when it
// starts. Defaults to false.
func (WinRTConfig) ExperimentalReinitializeWinRTApartmentOnMainThread() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigWinRTReinitialize() })
}

// SetExperimentalReinitializeWinRTApartmentOnMainThread sets whether the
// backend reinitializes the WinRT apartment on the calling thread when it starts.
func (WinRTConfig) SetExperimentalReinitializeWinRTApartmentOnMainThread(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigWinRTSetReinitialize(enabled) })
}

// UseDeferredDisconnect reports whether Disconnect returns immediately and
// cleans up in the background instead of blocking. Defaults to true.
func (WinRTConfig) UseDeferredDisconnect() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigWinRTDeferredDisconnect() })
}

// SetUseDeferredDisconnect sets whether Disconnect returns immediately and
// cleans up in the background.
func (WinRTConfig) SetUseDeferredDisconnect(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigWinRTSetDeferredDisconnect(enabled) })
}

// Reset restores the CoreBluetooth defaults.
func (CoreBluetoothConfig) Reset() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigCoreBluetoothReset() })
}

// Reset restores the Android defaults.
func (AndroidConfig) Reset() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigAndroidReset() })
}

// NegotiateMTU reports whether MTU is negotiated on connect. Defaults to true.
func (AndroidConfig) NegotiateMTU() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigAndroidNegotiateMTU() })
}

func (AndroidConfig) SetNegotiateMTU(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigAndroidSetNegotiateMTU(enabled) })
}

// Reset restores the Dongl defaults.
func (DonglConfig) Reset() error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigDonglReset() })
}

// UseDonglBackend reports whether enumeration includes the Dongl backend.
// Defaults to true.
func (DonglConfig) UseDonglBackend() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigDonglBackend() })
}

// SetUseDonglBackend includes or leaves out the Dongl backend, whose identifier
// is "Dongl", and its USB adapters.
func (DonglConfig) SetUseDonglBackend(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigDonglSetBackend(enabled) })
}

// AutoUpdate reports the firmware auto-update setting. Defaults to false.
func (DonglConfig) AutoUpdate() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigDonglAutoUpdate() })
}

// SetAutoUpdate is reserved for Dongl firmware updates; the current backend
// ignores it.
func (DonglConfig) SetAutoUpdate(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigDonglSetAutoUpdate(enabled) })
}

// ForceUpdate reports the forced firmware update setting. Defaults to false.
func (DonglConfig) ForceUpdate() (bool, error) {
	return configValue(func(native *bindings.NativeAPI) bool { return native.ConfigDonglForceUpdate() })
}

// SetForceUpdate is reserved for Dongl firmware updates; the current backend
// ignores it.
func (DonglConfig) SetForceUpdate(enabled bool) error {
	return configure(func(native *bindings.NativeAPI) { native.ConfigDonglSetForceUpdate(enabled) })
}
