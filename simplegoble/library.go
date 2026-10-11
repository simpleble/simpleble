package simplegoble

import "github.com/simpleble/simplegoble/internal/bindings"

// Version returns the loaded native SimpleBLE version.
func Version() (string, error) { return bindings.Version() }
