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
	adapters, err := ble.GetAdapters()
	if err != nil {
		return err
	}
	defer func() {
		for _, adapter := range adapters {
			_ = adapter.Close()
		}
	}()
	if len(adapters) == 0 {
		fmt.Println("No Bluetooth adapters found.")
		return nil
	}
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
		fmt.Printf("%s (%s), powered: %t\n", name, address, powered)
	}
	return nil
}
