package main

import (
	"fmt"
	"log"

	ble "github.com/simpleble/simplegoble"
)

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() error {
	backends, err := ble.GetBackends()
	if err != nil {
		return err
	}
	defer func() {
		for _, backend := range backends {
			_ = backend.Close()
		}
	}()
	for _, backend := range backends {
		name, err := backend.Identifier()
		if err != nil {
			return err
		}
		fmt.Println("Backend:", name)
		adapters, err := backend.Adapters()
		if err != nil {
			return err
		}
		defer func() {
			for _, adapter := range adapters {
				_ = adapter.Close()
			}
		}()
		for _, adapter := range adapters {
			name, err := adapter.Identifier()
			if err != nil {
				return err
			}
			address, err := adapter.Address()
			if err != nil {
				return err
			}
			powered, err := adapter.IsPowered()
			if err != nil {
				return err
			}
			fmt.Printf("  %s (%s), powered: %t\n", name, address, powered)
		}
	}
	return nil
}
