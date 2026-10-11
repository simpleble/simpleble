package simplegoble_test

import (
	"errors"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"testing"

	"github.com/simpleble/simplegoble/internal/bindings"
)

func plainLibrary(t *testing.T) string {
	t.Helper()
	root, err := filepath.Abs(filepath.Join("..", "..", "install_simplegoble_plain"))
	if err != nil {
		t.Fatal(err)
	}
	if _, err := os.Stat(root); err != nil {
		if os.IsNotExist(err) {
			t.Skip("build and install the PLAIN library to run native integration tests")
		}
		t.Fatal(err)
	}
	library := "lib/libsimplecble.so"
	switch runtime.GOOS {
	case "darwin":
		library = "lib/libsimplecble.dylib"
	case "windows":
		library = "bin/simplecble.dll"
	}
	return filepath.Join(root, filepath.FromSlash(library))
}

func TestPlainNativeErrors(t *testing.T) {
	path := plainLibrary(t)
	if err := bindings.Load(path); err != nil {
		t.Fatal(err)
	}
	native, err := bindings.API()
	if err != nil {
		t.Fatal(err)
	}
	var nativeError uintptr
	if native.BackendGet(^uintptr(0), &nativeError) != 0 {
		t.Fatal("invalid index returned a handle")
	}
	err = native.TakeError(&nativeError)
	var detail *bindings.Error
	if !errors.Is(err, bindings.InvalidArgument) || !errors.As(err, &detail) || !strings.Contains(detail.Message, "index") {
		t.Fatal(err)
	}
	if nativeError != 0 {
		t.Fatal("native error was not released and cleared")
	}
	saved := strings.Clone(detail.Message)
	// A different error must not overwrite the diagnostic retained by Go.
	if native.BackendIdentifier(0, &nativeError) != nil {
		t.Fatal("null backend returned an identifier")
	}
	if err := native.TakeError(&nativeError); !errors.Is(err, bindings.InvalidArgument) {
		t.Fatal(err)
	}
	if detail.Message != saved {
		t.Fatal("native error message was not copied")
	}
}
