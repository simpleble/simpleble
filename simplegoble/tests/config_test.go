package simplegoble_test

import (
	"errors"
	"testing"
	"time"

	ble "github.com/simpleble/simplegoble"
	"github.com/simpleble/simplegoble/internal/bindings"
)

func TestPlainConfiguration(t *testing.T) {
	if err := bindings.Load(plainLibrary(t)); err != nil {
		t.Fatal(err)
	}
	config := ble.Config
	t.Cleanup(func() { _ = config.ResetAll() })

	flags := []struct {
		name         string
		get          func() (bool, error)
		set          func(bool) error
		defaultValue bool
	}{
		{"UseSystemBus", config.SimpleBluez.UseSystemBus, config.SimpleBluez.SetUseSystemBus, true},
		{"ExperimentalUseOwnMTAApartment", config.WinRT.ExperimentalUseOwnMTAApartment, config.WinRT.SetExperimentalUseOwnMTAApartment, true},
		{"ExperimentalReinitializeWinRTApartmentOnMainThread", config.WinRT.ExperimentalReinitializeWinRTApartmentOnMainThread,
			config.WinRT.SetExperimentalReinitializeWinRTApartmentOnMainThread, false},
		{"UseDeferredDisconnect", config.WinRT.UseDeferredDisconnect, config.WinRT.SetUseDeferredDisconnect, true},
		{"UseDonglBackend", config.Dongl.UseDonglBackend, config.Dongl.SetUseDonglBackend, false},
		{"AutoUpdate", config.Dongl.AutoUpdate, config.Dongl.SetAutoUpdate, false},
		{"ForceUpdate", config.Dongl.ForceUpdate, config.Dongl.SetForceUpdate, false},
	}

	// Change every setting, so that a function wired to the wrong native setting shows up.
	for _, flag := range flags {
		if err := flag.set(!flag.defaultValue); err != nil {
			t.Fatal(err)
		}
		if value, err := flag.get(); err != nil || value == flag.defaultValue {
			t.Fatalf("%s = %v, %v after changing it", flag.name, value, err)
		}
	}
	if err := config.SimpleBluez.SetConnectionTimeout(7 * time.Second); err != nil {
		t.Fatal(err)
	}
	if err := config.SimpleBluez.SetDisconnectionTimeout(3 * time.Second); err != nil {
		t.Fatal(err)
	}
	if timeout, err := config.SimpleBluez.ConnectionTimeout(); err != nil || timeout != 7*time.Second {
		t.Fatalf("ConnectionTimeout = %v, %v", timeout, err)
	}
	if timeout, err := config.SimpleBluez.DisconnectionTimeout(); err != nil || timeout != 3*time.Second {
		t.Fatalf("DisconnectionTimeout = %v, %v", timeout, err)
	}
	if err := config.SimpleBluez.SetConnectionTimeout(-time.Second); !errors.Is(err, ble.InvalidArgument) {
		t.Fatal(err)
	}

	if err := config.ResetAll(); err != nil {
		t.Fatal(err)
	}
	for _, flag := range flags {
		if value, err := flag.get(); err != nil || value != flag.defaultValue {
			t.Fatalf("%s = %v, %v after reset", flag.name, value, err)
		}
	}
	if timeout, err := config.SimpleBluez.ConnectionTimeout(); err != nil || timeout != 2*time.Second {
		t.Fatalf("ConnectionTimeout = %v, %v after reset", timeout, err)
	}
}
