package main

import (
	"fmt"
	"log"
	"time"

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
		return fmt.Errorf("no Bluetooth adapters found")
	}
	adapter := adapters[0]
	if err := adapter.ScanFor(5 * time.Second); err != nil {
		return err
	}
	peripherals, err := adapter.ScanGetResults()
	if err != nil {
		return err
	}
	defer func() {
		for _, peripheral := range peripherals {
			_ = peripheral.Close()
		}
	}()
	for _, peripheral := range peripherals {
		name, err := peripheral.Identifier()
		if err != nil {
			return err
		}
		address, err := peripheral.Address()
		if err != nil {
			return err
		}
		rssi, err := peripheral.RSSI()
		if err != nil {
			return err
		}
		fmt.Printf("%s (%s), RSSI: %d dBm\n", name, address, rssi)
	}
	return nil
}
