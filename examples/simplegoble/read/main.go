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

// choose asks the user to pick one of count listed entries.
func choose(input *bufio.Reader, count int) (int, error) {
	fmt.Print("Enter choice: ")
	line, err := input.ReadString('\n')
	if err != nil {
		return 0, err
	}
	choice, err := strconv.Atoi(strings.TrimSpace(line))
	if err != nil || choice < 0 || choice >= count {
		return 0, fmt.Errorf("invalid choice %q", strings.TrimSpace(line))
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
		return fmt.Errorf("no adapters found")
	}

	// Query the user to pick an adapter
	fmt.Println("Please select an adapter:")
	for i, adapter := range adapters {
		fmt.Printf("%d: %s\n", i, describe(adapter.Identifier, adapter.Address))
	}
	choice, err := choose(input, len(adapters))
	if err != nil {
		return err
	}
	adapter := adapters[choice]
	fmt.Printf("Selected adapter: %s\n", describe(adapter.Identifier, adapter.Address))

	if err := adapter.SetCallbackOnScanStart(func() { fmt.Println("Scan started.") }); err != nil {
		return err
	}
	if err := adapter.SetCallbackOnScanStop(func() { fmt.Println("Scan complete.") }); err != nil {
		return err
	}
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
	type pair struct{ service, characteristic string }
	var pairs []pair
	for _, service := range services {
		for _, characteristic := range service.Characteristics {
			pairs = append(pairs, pair{service.UUID, characteristic.UUID})
		}
	}
	if len(pairs) == 0 {
		return fmt.Errorf("no characteristics found")
	}

	// Query the user to pick a service/characteristic pair
	fmt.Println("Please select a service/characteristic pair:")
	for i, p := range pairs {
		fmt.Printf("%d: %s %s\n", i, p.service, p.characteristic)
	}
	choice, err = choose(input, len(pairs))
	if err != nil {
		return err
	}

	contents, err := peripheral.Read(pairs[choice].service, pairs[choice].characteristic)
	if err != nil {
		return err
	}
	fmt.Printf("Contents: %X\n", contents)
	return nil
}
