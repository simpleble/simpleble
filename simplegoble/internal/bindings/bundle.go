package bindings

import (
	"bytes"
	"crypto/sha256"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"runtime"
)

// CI generates one file per target assigning these from a go:embed filesystem.
// The development checkout has no bundle; native tests load a PLAIN build.
var bundledLibraries fs.FS

// LibraryFile contains a native library's filename and contents.
type LibraryFile struct {
	Name string
	Data []byte
}

func bundledLibraryPath() (string, error) {
	libraryName := "libsimplecble.so"
	switch runtime.GOOS {
	case "darwin":
		libraryName = "libsimplecble.dylib"
	case "windows":
		libraryName = "simplecble.dll"
	}
	directory := "native/" + runtime.GOOS + "_" + runtime.GOARCH
	entries, err := fs.ReadDir(bundledLibraries, directory)
	if err != nil {
		return "", err
	}
	files := make([]LibraryFile, 0, len(entries))
	found := false
	for _, entry := range entries {
		name := entry.Name()
		data, err := fs.ReadFile(bundledLibraries, directory+"/"+name)
		if err != nil {
			return "", err
		}
		files = append(files, LibraryFile{name, data})
		found = found || name == libraryName
	}
	if !found {
		return "", fmt.Errorf("bundle is missing %s", libraryName)
	}
	cache, err := os.UserCacheDir()
	if err != nil {
		return "", err
	}
	dir, err := ExtractLibraries(filepath.Join(cache, "simplegoble"), files)
	if err != nil {
		return "", err
	}
	return filepath.Join(dir, libraryName), nil
}

// Publish a complete, private directory atomically. Multiple programs using the
// same bundle share a content-addressed cache. No network access is involved.
func ExtractLibraries(root string, files []LibraryFile) (string, error) {
	digest := sha256.New()
	for _, file := range files {
		fmt.Fprintf(digest, "%s\x00%d\x00", file.Name, len(file.Data))
		digest.Write(file.Data)
	}
	dir := filepath.Join(root, fmt.Sprintf("%x", digest.Sum(nil)))
	if _, err := os.Lstat(dir); err == nil {
		return dir, verifyLibraries(dir, files)
	} else if !os.IsNotExist(err) {
		return "", err
	}
	if err := os.MkdirAll(root, 0700); err != nil {
		return "", err
	}
	stage, err := os.MkdirTemp(root, ".extract-")
	if err != nil {
		return "", err
	}
	defer os.RemoveAll(stage)
	for _, file := range files {
		if err := os.WriteFile(filepath.Join(stage, file.Name), file.Data, 0600); err != nil {
			return "", err
		}
	}
	if err := os.Rename(stage, dir); err != nil {
		// Another process may have published the same complete bundle first.
		if verifyErr := verifyLibraries(dir, files); verifyErr != nil {
			return "", fmt.Errorf("publish native cache: %w", err)
		}
	}
	return dir, verifyLibraries(dir, files)
}

// Never load substituted cache contents. Files are checked against bytes already
// authenticated as part of the Go module and embedded in the application.
func verifyLibraries(dir string, files []LibraryFile) error {
	info, err := os.Lstat(dir)
	if err != nil {
		return err
	}
	if !info.IsDir() {
		return fmt.Errorf("native cache %q is not a directory", dir)
	}
	for _, file := range files {
		path := filepath.Join(dir, file.Name)
		info, err := os.Lstat(path)
		if err != nil {
			return err
		}
		if !info.Mode().IsRegular() {
			return fmt.Errorf("native cache file %q is not a regular file", path)
		}
		data, err := os.ReadFile(path)
		if err != nil {
			return err
		}
		if !bytes.Equal(data, file.Data) {
			return fmt.Errorf("native cache file %q differs from embedded library; remove this cache directory and retry", path)
		}
	}
	return nil
}
