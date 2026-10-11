package main

import (
	"fmt"
	"log"
	"os"
	"strings"
	"time"

	ble "github.com/simpleble/simplegoble"
)

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() error {
	if len(os.Args) != 4 {
		return fmt.Errorf("usage: read ADDRESS SERVICE_UUID CHARACTERISTIC_UUID")
	}
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
		address, err := peripheral.Address()
		if err != nil {
			return err
		}
		if !strings.EqualFold(address, os.Args[1]) {
			continue
		}
		if err := peripheral.Connect(); err != nil {
			return err
		}
		defer peripheral.Disconnect()
		data, err := peripheral.Read(os.Args[2], os.Args[3])
		if err != nil {
			return err
		}
		fmt.Printf("%X\n", data)
		return nil
	}
	return fmt.Errorf("peripheral %q not found", os.Args[1])
}
