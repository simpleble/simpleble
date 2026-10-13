// Package simplegoble provides Go bindings for SimpleBLE on Windows, macOS, and Linux.
//
// Discover and connect peripherals, read and write GATT values, and subscribe
// to notifications and indications, or host local GATT services. Close every
// owned Backend, Adapter, Peripheral, LocalPeripheral, LocalService, and
// LocalCharacteristic handle when finished. As with os.File, a handle that
// becomes unreachable without Close is eventually released, except a
// LocalPeripheral, which must be closed to stop hosting. A handle with
// registered callbacks stays reachable until they are cleared or it is closed.
// Copies of a Go wrapper share ownership; closing any copy closes them all.
// Methods are safe for concurrent use, and Close waits for native calls on
// that wrapper.
//
// Set backend options, such as enabling Dongl adapters, through Config before
// the first GetBackends, GetAdapters, or BluetoothEnabled call.
package simplegoble
