//go:build amd64 || arm64

package bindings

import (
	"syscall"
	"unsafe"
)

var loadLibraryEx = syscall.NewLazyDLL("kernel32.dll").NewProc("LoadLibraryExW")

func openLibrary(path string) (uintptr, error) {
	nativePath, err := syscall.UTF16PtrFromString(path)
	if err != nil {
		return 0, err
	}
	// Resolve sibling DLLs even when Load points outside the executable's
	// directory, and exclude the working directory from dependency lookup.
	// https://learn.microsoft.com/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw
	const searchDLLLoadDir = 0x00000100
	const searchDefaultDirs = 0x00001000
	libraryHandle, _, err := loadLibraryEx.Call(uintptr(unsafe.Pointer(nativePath)), 0, searchDLLLoadDir|searchDefaultDirs)
	if libraryHandle == 0 {
		return 0, err
	}
	return libraryHandle, nil
}
func findSymbol(libraryHandle uintptr, name string) (uintptr, error) {
	return syscall.GetProcAddress(syscall.Handle(libraryHandle), name)
}
func closeLibrary(libraryHandle uintptr) { _ = syscall.FreeLibrary(syscall.Handle(libraryHandle)) }
