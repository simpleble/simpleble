// Package simplegoble provides Go bindings for SimpleBLE on Windows, macOS, and Linux.
//
// Discover and connect peripherals, read and write GATT values, and subscribe
// to notifications and indications. Every Backend, Adapter, and Peripheral
// returned by enumeration or discovery callbacks must be closed explicitly.
// Copies of a Go
// wrapper share ownership; closing any copy closes them all. Methods are safe
// for concurrent use, and Close waits for native calls on that wrapper.
package simplegoble
