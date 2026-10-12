//go:build (darwin || linux) && (amd64 || arm64)

package bindings

import "github.com/ebitengine/purego"

func openLibrary(path string) (uintptr, error) {
	return purego.Dlopen(path, purego.RTLD_NOW|purego.RTLD_LOCAL)
}
func findSymbol(libraryHandle uintptr, name string) (uintptr, error) {
	return purego.Dlsym(libraryHandle, name)
}
func closeLibrary(libraryHandle uintptr) { _ = purego.Dlclose(libraryHandle) }

// currentThread identifies the native thread; Go has no thread-local storage.
var currentThread func() uintptr

func init() { purego.RegisterLibFunc(&currentThread, purego.RTLD_DEFAULT, "pthread_self") }
