// Package simplegoble provides Go bindings for SimpleBLE on Windows, macOS, and Linux.
//
// Enumerate Bluetooth backends and adapters, control power and scanning, and
// register callbacks for power and scan events. Every Backend and Adapter
// returned by enumeration must be closed explicitly. Copies of a Go
// wrapper share ownership; closing any copy closes them all. Methods are safe
// for concurrent use, and Close waits for native calls on that wrapper.
package simplegoble
