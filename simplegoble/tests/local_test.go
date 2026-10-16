package simplegoble_test

import (
	"bytes"
	"errors"
	"runtime"
	"testing"
	"time"

	ble "github.com/simpleble/simplegoble"
)

const (
	hostService = "12345678-1234-5678-1234-56789abcdef0"
	hostValue   = "12345678-1234-5678-1234-56789abcdef1"
)

func plainHost(t *testing.T) (*ble.Adapter, *ble.LocalPeripheral, *ble.LocalService, *ble.LocalCharacteristic) {
	t.Helper()
	a := plainAdapter(t)
	host, err := a.CreateLocalPeripheral()
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = host.Close() })
	if err := host.AddAdvertisedService(hostService); err != nil {
		t.Fatal(err)
	}
	service, err := host.AddService(hostService)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = service.Close() })
	c, err := service.AddCharacteristic(hostValue, ble.CapabilityRead|ble.CapabilityWriteRequest|
		ble.CapabilityWriteCommand|ble.CapabilityNotify|ble.CapabilityIndicate)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = c.Close() })
	return a, host, service, c
}

func connectHost(t *testing.T, a *ble.Adapter, host *ble.LocalPeripheral) *ble.Peripheral {
	t.Helper()
	p := findHost(t, a, host)
	if err := p.Connect(); err != nil {
		t.Fatal(err)
	}
	return p
}

// findHost starts the host and scans until it is found.
func findHost(t *testing.T, a *ble.Adapter, host *ble.LocalPeripheral) *ble.Peripheral {
	t.Helper()
	found := make(chan *ble.Peripheral, 1)
	sent := false
	if err := a.SetCallbackOnScanFound(func(p *ble.Peripheral) {
		services, err := p.Services()
		if err == nil && !sent {
			for _, service := range services {
				if service.UUID == hostService {
					sent = true
					found <- p
					return
				}
			}
		}
		_ = p.Close()
	}); err != nil {
		t.Fatal(err)
	}
	if err := host.Start(); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStart(); err != nil {
		t.Fatal(err)
	}
	p := receive(t, found)
	t.Cleanup(func() { _ = p.Disconnect(); _ = p.Unpair(); _ = p.Close() })
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	if err := a.SetCallbackOnScanFound(nil); err != nil {
		t.Fatal(err)
	}
	return p
}

func TestPlainLocalManufacturerDataAndConnectability(t *testing.T) {
	a, host, _, _ := plainHost(t)
	if err := host.SetAdvertisedManufacturerData(0x004c, []byte{0x02}); err != nil {
		t.Fatal(err)
	}
	if err := host.SetAdvertisedManufacturerData(0x004c, []byte{0x02, 0x15}); err != nil {
		t.Fatal(err)
	}
	if err := host.SetConnectable(false); err != nil {
		t.Fatal(err)
	}

	p := findHost(t, a, host)
	data, err := p.ManufacturerData()
	if err != nil || len(data) != 1 || !bytes.Equal(data[0x004c], []byte{0x02, 0x15}) {
		t.Fatalf("manufacturer data = %v, %v", data, err)
	}
	if connectable, err := p.IsConnectable(); err != nil || connectable {
		t.Fatalf("connectable = %v, %v", connectable, err)
	}
}

func TestPlainLocalConfigurationAndValueOwnership(t *testing.T) {
	_, host, service, c := plainHost(t)
	input := []byte{0, 0x7f, 0x80, 0xff}
	if err := c.SetValue(input); err != nil {
		t.Fatal(err)
	}
	input[0] = 99
	value, err := c.Value()
	if err != nil || !bytes.Equal(value, []byte{0, 0x7f, 0x80, 0xff}) {
		t.Fatalf("stored value = %v, %v", value, err)
	}
	value[0] = 88
	if fresh, err := c.Value(); err != nil || fresh[0] != 0 {
		t.Fatalf("retained value aliases native bytes: %v, %v", fresh, err)
	}
	if uuid, err := service.UUID(); err != nil || uuid != hostService {
		t.Fatalf("service UUID = %q, %v", uuid, err)
	}
	if uuid, err := c.UUID(); err != nil || uuid != hostValue {
		t.Fatalf("characteristic UUID = %q, %v", uuid, err)
	}
	if flags, err := c.Capabilities(); err != nil || flags != 31 {
		t.Fatalf("capabilities = %v, %v", flags, err)
	}
	if _, err := service.AddCharacteristic("0000fff2-0000-1000-8000-00805f9b34fb", 1<<31); !errors.Is(err, ble.InvalidArgument) {
		t.Fatal(err)
	}
	if _, err := host.AddService("180f\x00"); !errors.Is(err, ble.InvalidArgument) {
		t.Fatal(err)
	}
	// Repeated UUIDs follow the native backend; PLAIN rejects them.
	if _, err := host.AddService(hostService); !errors.Is(err, ble.OperationFailed) {
		t.Fatalf("repeated service = %v", err)
	}
	if _, err := service.AddCharacteristic(hostValue, ble.CapabilityRead); !errors.Is(err, ble.OperationFailed) {
		t.Fatalf("repeated characteristic = %v", err)
	}
	if err := host.Start(); err != nil {
		t.Fatal(err)
	}
	if started, err := host.IsStarted(); err != nil || !started {
		t.Fatalf("started = %v, %v", started, err)
	}
	if advertising, err := host.IsAdvertising(); err != nil || !advertising {
		t.Fatalf("advertising = %v, %v", advertising, err)
	}
	if err := host.RemoveAllServices(); !errors.Is(err, ble.OperationFailed) {
		t.Fatal(err)
	}
	if _, err := service.AddCharacteristic("0000fff2-0000-1000-8000-00805f9b34fb", ble.CapabilityRead); !errors.Is(err, ble.OperationFailed) {
		t.Fatal(err)
	}
	if err := host.Stop(); err != nil {
		t.Fatal(err)
	}
	if err := host.RemoveAllServices(); err != nil {
		t.Fatal(err)
	}
	if services, err := host.Services(); err != nil || len(services) != 0 {
		t.Fatalf("removed services = %v, %v", services, err)
	}
	if err := c.SetValue(nil); !errors.Is(err, ble.GATTServiceNotFound) {
		t.Fatal(err)
	}
	copy := *host
	if err := host.Close(); err != nil {
		t.Fatal(err)
	}
	if copy.Initialized() {
		t.Fatal("copy retained the released host")
	}
	if err := copy.Start(); !errors.Is(err, ble.ErrClosed) {
		t.Fatal(err)
	}
}

func TestPlainLocalReadCallbacksAndIndependentHandles(t *testing.T) {
	a, host, service, c := plainHost(t)
	stored := []byte{9, 0, 0xff}
	if err := c.SetValue(stored); err != nil {
		t.Fatal(err)
	}
	services, err := host.Services()
	if err != nil || len(services) != 1 {
		t.Fatalf("services = %v, %v", services, err)
	}
	defer services[0].Close()
	if err := service.Close(); err != nil {
		t.Fatal(err)
	}
	characteristics, err := services[0].Characteristics()
	if err != nil || len(characteristics) != 1 {
		t.Fatalf("characteristics = %v, %v", characteristics, err)
	}
	other := characteristics[0]
	defer other.Close()
	if err := c.SetCallbackOnRead(func() []byte { return []byte{1} }); err != nil {
		t.Fatal(err)
	}
	if err := other.SetCallbackOnRead(func() []byte { return []byte{0, 0xff, 2} }); err != nil {
		t.Fatal(err)
	}
	if err := c.Close(); err != nil {
		t.Fatal(err)
	}
	p := connectHost(t, a, host)
	for i := 0; i < 8; i++ {
		runtime.GC()
		if data, err := p.Read(hostService, hostValue); err != nil || !bytes.Equal(data, []byte{0, 0xff, 2}) {
			t.Fatalf("replacement read = %v, %v", data, err)
		}
	}
	if err := other.SetCallbackOnRead(func() []byte { return nil }); err != nil {
		t.Fatal(err)
	}
	if data, err := p.Read(hostService, hostValue); err != nil || len(data) != 0 {
		t.Fatalf("empty read = %v, %v", data, err)
	}
	if err := other.SetCallbackOnRead(nil); err != nil {
		t.Fatal(err)
	}
	if data, err := p.Read(hostService, hostValue); err != nil || !bytes.Equal(data, stored) {
		t.Fatalf("stored read = %v, %v", data, err)
	}
	entered, release := make(chan struct{}), make(chan struct{})
	defer func() {
		select {
		case <-release:
		default:
			close(release)
		}
	}()
	if err := other.SetCallbackOnRead(func() []byte {
		close(entered)
		<-release
		return []byte{0x80, 0, 0xff}
	}); err != nil {
		t.Fatal(err)
	}
	result := make(chan []byte, 1)
	readError := make(chan error, 1)
	go func() {
		data, err := p.Read(hostService, hostValue)
		result <- data
		readError <- err
	}()
	receive(t, entered)
	closed := make(chan error, 1)
	go func() { closed <- other.Close() }()
	runtime.GC()
	close(release)
	if data := receive(t, result); !bytes.Equal(data, []byte{0x80, 0, 0xff}) {
		t.Fatalf("in-flight read = %v", data)
	}
	if err := receive(t, readError); err != nil {
		t.Fatal(err)
	}
	if err := receive(t, closed); err != nil {
		t.Fatal(err)
	}
	if data, err := p.Read(hostService, hostValue); err != nil || !bytes.Equal(data, stored) {
		t.Fatalf("close did not restore stored read: %v, %v", data, err)
	}
}

func TestPlainLocalAliasesFollowUUIDsNotPositions(t *testing.T) {
	a, host, _, c := plainHost(t)
	// PLAIN lists services by UUID, so this one moves ahead of hostService.
	first, err := host.AddService("0000aaaa-0000-1000-8000-00805f9b34fb")
	if err != nil {
		t.Fatal(err)
	}
	defer first.Close()
	if err := c.SetCallbackOnRead(func() []byte { return []byte{1} }); err != nil {
		t.Fatal(err)
	}
	services, err := host.Services()
	if err != nil || len(services) != 2 {
		t.Fatalf("services = %v, %v", services, err)
	}
	var alias *ble.LocalCharacteristic
	for _, service := range services {
		defer service.Close()
		if uuid, err := service.UUID(); err != nil || uuid != hostService {
			continue
		}
		characteristics, err := service.Characteristics()
		if err != nil || len(characteristics) != 1 {
			t.Fatalf("characteristics = %v, %v", characteristics, err)
		}
		alias = characteristics[0]
		defer alias.Close()
	}
	if alias == nil {
		t.Fatal("hosted service was not enumerated")
	}
	if err := alias.SetCallbackOnRead(func() []byte { return []byte{2} }); err != nil {
		t.Fatal(err)
	}
	// Closing the original must not clear the alias's replacement read callback.
	if err := c.Close(); err != nil {
		t.Fatal(err)
	}
	p := connectHost(t, a, host)
	if data, err := p.Read(hostService, hostValue); err != nil || !bytes.Equal(data, []byte{2}) {
		t.Fatalf("alias read = %v, %v", data, err)
	}
}

func TestPlainLocalReadHandlerCannotReplaceOrCloseItself(t *testing.T) {
	a, host, _, c := plainHost(t)
	rejected := make(chan error, 4)
	if err := c.SetCallbackOnRead(func() []byte {
		rejected <- c.SetCallbackOnRead(func() []byte { return []byte{2} })
		rejected <- c.Close()
		return []byte{1}
	}); err != nil {
		t.Fatal(err)
	}
	p := connectHost(t, a, host)
	for range 2 {
		if data, err := p.Read(hostService, hostValue); err != nil || !bytes.Equal(data, []byte{1}) {
			t.Fatalf("read = %v, %v", data, err)
		}
		for range 2 {
			if err := receive(t, rejected); !errors.Is(err, ble.OperationNotSupported) {
				t.Fatalf("change from read handler = %v", err)
			}
		}
	}
	if !c.Initialized() {
		t.Fatal("read handler closed its characteristic")
	}
	// A stranded pinned buffer panics when the collector finalizes it.
	for range 5 {
		runtime.GC()
		time.Sleep(10 * time.Millisecond)
	}
}

func TestPlainLocalWritesSubscriptionsAndClientEvents(t *testing.T) {
	a, host, _, c := plainHost(t)
	connections := make(chan string, 2)
	if err := host.SetCallbackOnClientConnected(func(address string) { connections <- "connected:" + address }); err != nil {
		t.Fatal(err)
	}
	if err := host.SetCallbackOnClientDisconnected(func(address string) { connections <- "disconnected:" + address }); err != nil {
		t.Fatal(err)
	}
	subscriptions := make(chan bool, 4)
	if err := c.SetCallbackOnSubscribed(func() { subscriptions <- true }); err != nil {
		t.Fatal(err)
	}
	if err := c.SetCallbackOnUnsubscribed(func() { subscriptions <- false }); err != nil {
		t.Fatal(err)
	}
	writes, callbackErrors := make(chan []byte, 2), make(chan error, 2)
	if err := c.SetCallbackOnWrite(func(data []byte) {
		callbackErrors <- c.SetValue(data)
		writes <- data
	}); err != nil {
		t.Fatal(err)
	}
	p := connectHost(t, a, host)
	if event := receive(t, connections); event != "connected:AA:BB:CC:DD:EE:FF" {
		t.Fatal(event)
	}
	values := make(chan []byte, 4)
	if err := p.Notify(hostService, hostValue, func(data []byte) { values <- data }); err != nil {
		t.Fatal(err)
	}
	if subscribed := receive(t, subscriptions); !subscribed {
		t.Fatal("subscription event missing")
	}
	for i, payload := range [][]byte{{0, 0xff, 0x80}, {}} {
		write := p.WriteRequest
		if i == 1 {
			write = p.WriteCommand
		}
		if err := write(hostService, hostValue, payload); err != nil {
			t.Fatal(err)
		}
		if data := receive(t, writes); !bytes.Equal(data, payload) {
			t.Fatalf("write callback = %v", data)
		}
		if err := receive(t, callbackErrors); err != nil {
			t.Fatal(err)
		}
		if data := receive(t, values); !bytes.Equal(data, payload) {
			t.Fatalf("echo notification = %v", data)
		}
	}
	if err := p.Unsubscribe(hostService, hostValue); err != nil {
		t.Fatal(err)
	}
	if subscribed := receive(t, subscriptions); subscribed {
		t.Fatal("unsubscribe event missing")
	}
	if err := p.Indicate(hostService, hostValue, func(data []byte) { values <- data }); err != nil {
		t.Fatal(err)
	}
	if subscribed := receive(t, subscriptions); !subscribed {
		t.Fatal("indication subscription missing")
	}
	if err := c.SetValue([]byte{7}); err != nil {
		t.Fatal(err)
	}
	if data := receive(t, values); !bytes.Equal(data, []byte{7}) {
		t.Fatalf("indication = %v", data)
	}
	if err := p.Disconnect(); err != nil {
		t.Fatal(err)
	}
	if event := receive(t, connections); event != "disconnected:AA:BB:CC:DD:EE:FF" {
		t.Fatal(event)
	}
	if subscribed := receive(t, subscriptions); subscribed {
		t.Fatal("disconnect left a subscription")
	}
}
