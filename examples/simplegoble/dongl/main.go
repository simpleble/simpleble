package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	ble "github.com/simpleble/simplegoble"
)

func main() {
	if err := run(); err != nil {
		log.Fatal(err)
	}
}

// ask prints a question and returns the user's answer.
func ask(input *bufio.Reader, question string) (string, error) {
	fmt.Print(question)
	line, err := input.ReadString('\n')
	return strings.TrimSpace(line), err
}

// choose asks the user to pick one of count listed entries.
func choose(input *bufio.Reader, count int) (int, error) {
	answer, err := ask(input, "Enter choice: ")
	if err != nil {
		return 0, err
	}
	choice, err := strconv.Atoi(answer)
	if err != nil || choice < 0 || choice >= count {
		return 0, fmt.Errorf("invalid choice %q", answer)
	}
	return choice, nil
}

func describe(name func() (string, error), address func() (string, error)) string {
	identifier, _ := name()
	mac, _ := address()
	return fmt.Sprintf("%s [%s]", identifier, mac)
}

func run() error {
	input := bufio.NewReader(os.Stdin)

	dongl, err := ble.GetDonglBackend()
	if err != nil {
		return err
	}
	defer dongl.Close()
	adapters, err := dongl.Adapters()
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

	// Query the user to pick a Dongl
	fmt.Println("Please select a Dongl:")
	for i, adapter := range adapters {
		fmt.Printf("%d: %s\n", i, describe(adapter.Identifier, adapter.Address))
	}
	choice, err := choose(input, len(adapters))
	if err != nil {
		return err
	}
	adapter := adapters[choice]
	fmt.Printf("Selected Dongl: %s\n", describe(adapter.Identifier, adapter.Address))

	if err := adapter.SetCallbackOnScanFound(func(peripheral *ble.Peripheral) {
		fmt.Printf("Found %s\n", describe(peripheral.Identifier, peripheral.Address))
		_ = peripheral.Close()
	}); err != nil {
		return err
	}

	// Scan for 5 seconds
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
	if len(peripherals) == 0 {
		return fmt.Errorf("no peripherals found")
	}

	// Query the user to pick a peripheral
	fmt.Println("Please select a peripheral:")
	for i, peripheral := range peripherals {
		fmt.Printf("%d: %s\n", i, describe(peripheral.Identifier, peripheral.Address))
	}
	choice, err = choose(input, len(peripherals))
	if err != nil {
		return err
	}
	peripheral := peripherals[choice]

	// Pairing can start as soon as the link is up, so register the callbacks
	// before connecting. The Dongl pairs without the operating system.
	if err := peripheral.SetCallbackOnPasskeyRequest(func() (string, bool) {
		passkey, err := ask(input, "Enter the passkey shown on the device (empty to reject): ")
		return passkey, err == nil && passkey != ""
	}); err != nil {
		return err
	}
	if err := peripheral.SetCallbackOnPasskeyDisplay(func(passkey string) {
		fmt.Printf("Enter %s on the device.\n", passkey)
	}); err != nil {
		return err
	}
	if err := peripheral.SetCallbackOnNumericComparison(func(passkey string) bool {
		answer, err := ask(input, fmt.Sprintf("Does the device show %s? [y/N] ", passkey))
		return err == nil && answer == "y"
	}); err != nil {
		return err
	}

	fmt.Printf("Connecting to: %s\n", describe(peripheral.Identifier, peripheral.Address))
	if err := peripheral.Connect(); err != nil {
		return err
	}
	defer peripheral.Disconnect()

	fmt.Println("Successfully connected, listing services...")
	services, err := peripheral.Services()
	if err != nil {
		return err
	}
	for _, service := range services {
		fmt.Printf("Service: %s\n", service.UUID)
		for _, characteristic := range service.Characteristics {
			fmt.Printf("    Characteristic: %s\n", characteristic.UUID)
		}
	}

	paired, err := peripheral.IsPaired()
	if err != nil {
		return err
	}
	fmt.Printf("Paired: %t\n", paired)
	return nil
}
