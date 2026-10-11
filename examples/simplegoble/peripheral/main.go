package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"

	ble "github.com/simpleble/simplegoble"
)

const (
	serviceUUID        = "12345678-1234-5678-1234-56789abcdef0"
	characteristicUUID = "12345678-1234-5678-1234-56789abcdef1"
)

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() error {
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()
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
	host, err := adapters[0].CreateLocalPeripheral()
	if err != nil {
		return err
	}
	defer host.Close()
	if err := host.AddAdvertisedService(serviceUUID); err != nil {
		return err
	}
	service, err := host.AddService(serviceUUID)
	if err != nil {
		return err
	}
	defer service.Close()
	characteristic, err := service.AddCharacteristic(characteristicUUID,
		ble.CapabilityRead|ble.CapabilityWriteRequest|ble.CapabilityWriteCommand|ble.CapabilityNotify|ble.CapabilityIndicate)
	if err != nil {
		return err
	}
	defer characteristic.Close()
	if err := characteristic.SetValue([]byte("ready")); err != nil {
		return err
	}
	if err := characteristic.SetCallbackOnWrite(func(value []byte) {
		fmt.Printf("Write: %X\n", value)
		if err := characteristic.SetValue(value); err != nil {
			log.Printf("Echo failed: %v", err)
		}
	}); err != nil {
		return err
	}
	if err := host.SetCallbackOnClientConnected(func(address string) {
		fmt.Println("Client connected:", address)
	}); err != nil {
		return err
	}
	if err := host.SetCallbackOnClientDisconnected(func(address string) {
		fmt.Println("Client disconnected:", address)
	}); err != nil {
		return err
	}
	if err := host.Start(); err != nil {
		return err
	}
	defer host.Stop()
	fmt.Println("Advertising. Press Ctrl+C to stop.")
	<-ctx.Done()
	return nil
}
