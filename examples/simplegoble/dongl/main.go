package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
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
	if len(os.Args) > 2 {
		return fmt.Errorf("usage: dongl [ADDRESS]")
	}
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()
	// Enable the Dongl backend before any other SimpleGoBLE call.
	if err := ble.Config.Dongl.SetUseDonglBackend(true); err != nil {
		return err
	}
	adapters, err := donglAdapters()
	if err != nil {
		return err
	}
	defer func() {
		for _, adapter := range adapters {
			_ = adapter.Close()
		}
	}()
	if len(adapters) == 0 {
		return fmt.Errorf("no Dongl found")
	}
	adapter := adapters[0]
	name, err := adapter.Identifier()
	if err != nil {
		return err
	}
	address, err := adapter.Address()
	if err != nil {
		return err
	}
	fmt.Printf("%s [%s]\n", name, address)

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
	var target *ble.Peripheral
	for _, peripheral := range peripherals {
		name, err := peripheral.Identifier()
		if err != nil {
			return err
		}
		address, err := peripheral.Address()
		if err != nil {
			return err
		}
		fmt.Printf("%s (%s)\n", name, address)
		if len(os.Args) == 2 && strings.EqualFold(address, os.Args[1]) {
			target = peripheral
		}
	}
	if len(os.Args) == 1 {
		return nil
	}
	if target == nil {
		return fmt.Errorf("peripheral %q not found", os.Args[1])
	}
	return connect(ctx, target)
}

// donglAdapters returns the adapters of the backend whose identifier is "Dongl".
func donglAdapters() ([]*ble.Adapter, error) {
	backends, err := ble.GetBackends()
	if err != nil {
		return nil, err
	}
	defer func() {
		for _, backend := range backends {
			_ = backend.Close()
		}
	}()
	for _, backend := range backends {
		name, err := backend.Identifier()
		if err != nil {
			return nil, err
		}
		if name == "Dongl" {
			return backend.Adapters()
		}
	}
	return nil, nil
}

// connect answers pairing requests from the terminal, which the Dongl handles
// without the operating system. Register the callbacks before connecting.
func connect(ctx context.Context, peripheral *ble.Peripheral) error {
	input := bufio.NewReader(os.Stdin)
	ask := func(question string) (string, bool) {
		fmt.Print(question)
		answer, err := input.ReadString('\n')
		return strings.TrimSpace(answer), err == nil
	}
	if err := peripheral.SetCallbackOnNumericComparison(func(passkey string) bool {
		answer, _ := ask(fmt.Sprintf("Does the device show %s? [y/N] ", passkey))
		return answer == "y"
	}); err != nil {
		return err
	}
	if err := peripheral.SetCallbackOnPasskeyRequest(func() (string, bool) {
		return ask("Enter the passkey shown by the device: ")
	}); err != nil {
		return err
	}
	if err := peripheral.SetCallbackOnPasskeyDisplay(func(passkey string) {
		fmt.Printf("Enter %s on the device.\n", passkey)
	}); err != nil {
		return err
	}
	if err := peripheral.Connect(); err != nil {
		return err
	}
	defer peripheral.Disconnect()
	paired, err := peripheral.IsPaired()
	if err != nil {
		return err
	}
	fmt.Printf("Connected, paired: %t. Press Ctrl+C to disconnect.\n", paired)
	<-ctx.Done()
	return nil
}
