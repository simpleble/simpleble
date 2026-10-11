package simplegoble_test

import (
	"bytes"
	"errors"
	"testing"
	"time"

	ble "github.com/simpleble/simplegoble"
)

const (
	batteryService = "0000180f-0000-1000-8000-00805f9b34fb"
	batteryValue   = "00002a19-0000-1000-8000-00805f9b34fb"
	testService    = "0000fff0-0000-1000-8000-00805f9b34fb"
	testValue      = "0000fff1-0000-1000-8000-00805f9b34fb"
	description    = "00002901-0000-1000-8000-00805f9b34fb"
	cccd           = "00002902-0000-1000-8000-00805f9b34fb"
)

func plainPeripheral(t *testing.T, adapter *ble.Adapter) *ble.Peripheral {
	t.Helper()
	if err := adapter.ScanFor(250 * time.Millisecond); err != nil {
		t.Fatal(err)
	}
	peripherals, err := adapter.ScanGetResults()
	if err != nil {
		t.Fatal(err)
	}
	for _, p := range peripherals {
		t.Cleanup(func() { _ = p.Disconnect(); _ = p.Unpair(); _ = p.Close() })
	}
	if len(peripherals) != 1 {
		t.Fatalf("scan results = %d", len(peripherals))
	}
	p := peripherals[0]
	if err := p.Disconnect(); err != nil {
		t.Fatal(err)
	}
	return p
}

func TestPlainPeripheralDiscoveryAndOwnership(t *testing.T) {
	a := plainAdapter(t)
	p := plainPeripheral(t, a)
	found, updated := make(chan *ble.Peripheral, 8), make(chan *ble.Peripheral, 8)
	if err := a.SetCallbackOnScanFound(func(p *ble.Peripheral) { found <- p }); err != nil {
		t.Fatal(err)
	}
	if err := a.SetCallbackOnScanUpdated(func(p *ble.Peripheral) { updated <- p }); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanFor(250 * time.Millisecond); err != nil {
		t.Fatal(err)
	}
	// A scan-stop event drains the same native and Go queues as discovery events.
	barrier := make(chan struct{}, 1)
	if err := a.SetCallbackOnScanStop(func() { barrier <- struct{}{} }); err != nil {
		t.Fatal(err)
	}
	if err := a.ScanStop(); err != nil {
		t.Fatal(err)
	}
	receive(t, barrier)
	if err := a.SetCallbackOnScanFound(nil); err != nil {
		t.Fatal(err)
	}
	if err := a.SetCallbackOnScanUpdated(nil); err != nil {
		t.Fatal(err)
	}
	peer := receive(t, found)
	defer peer.Close()
	update := receive(t, updated)
	defer update.Close()
	for len(updated) > 0 {
		_ = (<-updated).Close()
	}
	if err := a.Close(); err != nil {
		t.Fatal(err)
	}
	if name, err := peer.Identifier(); err != nil || name != "Plain Peripheral" {
		t.Fatalf("discovered name = %q, %v", name, err)
	}
	if address, err := update.Address(); err != nil || address != "11:22:33:44:55:66" {
		t.Fatalf("updated address = %q, %v", address, err)
	}
	if kind, err := p.AddressType(); err != nil || kind != ble.AddressPublic {
		t.Fatalf("address type = %v, %v", kind, err)
	}
	if rssi, err := p.RSSI(); err != nil || rssi != -60 {
		t.Fatalf("RSSI = %v, %v", rssi, err)
	}
	if tx, err := p.TxPower(); err != nil || tx != 5 {
		t.Fatalf("TxPower = %v, %v", tx, err)
	}
	if connectable, err := p.IsConnectable(); err != nil || !connectable {
		t.Fatalf("connectable = %v, %v", connectable, err)
	}
	if err := peer.Connect(); err != nil {
		t.Fatal(err)
	}
	if connected, err := update.IsConnected(); err != nil || !connected {
		t.Fatalf("shared connection = %v, %v", connected, err)
	}
	copy := *peer
	if err := peer.Close(); err != nil {
		t.Fatal(err)
	}
	if copy.Initialized() {
		t.Fatal("copy retained released peripheral handle")
	}
	if _, err := copy.Read("180f", "2a19"); !errors.Is(err, ble.ErrClosed) {
		t.Fatal(err)
	}
	if connected, err := p.IsConnected(); err != nil || !connected {
		t.Fatalf("Close disconnected independent handle: %v, %v", connected, err)
	}
}

func TestPlainGATTValuesMetadataAndErrors(t *testing.T) {
	a := plainAdapter(t)
	p := plainPeripheral(t, a)
	advertised, err := p.Services()
	if err != nil || len(advertised) != 2 {
		t.Fatalf("advertised services = %v, %v", advertised, err)
	}
	if advertised[0].UUID != testService || !bytes.Equal(advertised[0].Data, []byte{0, 0x7f, 0x80, 0xff}) {
		t.Fatalf("advertised data = %v", advertised)
	}
	if err := p.Connect(); err != nil {
		t.Fatal(err)
	}
	services, err := p.Services()
	if err != nil || len(services) != 2 {
		t.Fatalf("GATT services = %v, %v", services, err)
	}
	var value ble.Characteristic
	for _, s := range services {
		for _, c := range s.Characteristics {
			if c.UUID == testValue {
				value = c
			}
		}
	}
	if !value.CanRead || !value.CanWriteRequest || !value.CanWriteCommand || !value.CanNotify || !value.CanIndicate || len(value.Descriptors) != 2 {
		t.Fatalf("characteristic layout = %+v", value)
	}
	if value.Descriptors[0].UUID != cccd || value.Descriptors[1].UUID != description {
		t.Fatalf("descriptor layout = %v", value.Descriptors)
	}
	manufacturer, err := p.ManufacturerData()
	if err != nil || !bytes.Equal(manufacturer[0x004c], []byte("test")) {
		t.Fatalf("manufacturer data = %v, %v", manufacturer, err)
	}
	if mtu, err := p.MTU(); err != nil || mtu != 244 {
		t.Fatalf("MTU = %v, %v", mtu, err)
	}
	if battery, err := p.Read("0000180F", "2A19"); err != nil || !bytes.Equal(battery, []byte{100}) {
		t.Fatalf("short uppercase UUID read = %v, %v", battery, err)
	}
	for _, write := range []func(string, string, []byte) error{p.WriteRequest, p.WriteCommand} {
		for _, payload := range [][]byte{{0, 0xff, 0x80, 1}, nil} {
			if err := write(testService, testValue, payload); err != nil {
				t.Fatal(err)
			}
			data, err := p.Read("fff0", "fff1")
			if err != nil || !bytes.Equal(data, payload) || data == nil {
				t.Fatalf("characteristic round trip = %v, %v", data, err)
			}
		}
	}
	for _, payload := range [][]byte{{0, 0xff, 0x80, 1}, {}} {
		if err := p.WriteDescriptor(testService, testValue, description, payload); err != nil {
			t.Fatal(err)
		}
		if data, err := p.ReadDescriptor("fff0", "fff1", "2901"); err != nil || !bytes.Equal(data, payload) || data == nil {
			t.Fatalf("descriptor round trip = %v, %v", data, err)
		}
	}
	// Error codes must survive the C boundary, and failures must leave the handle usable.
	for _, check := range []struct {
		call func() error
		code ble.ErrorCode
	}{
		{func() error { _, err := p.Read("invalid", testValue); return err }, ble.InvalidArgument},
		{func() error { _, err := p.Read("fff9", testValue); return err }, ble.GATTServiceNotFound},
		{func() error { _, err := p.Read(testService, "fff9"); return err }, ble.GATTCharacteristicNotFound},
		{func() error { _, err := p.ReadDescriptor(testService, testValue, "2909"); return err }, ble.GATTDescriptorNotFound},
		{func() error { _, err := p.Read(testService, "fff2"); return err }, ble.OperationFailed},
		{func() error { return p.WriteRequest(batteryService, batteryValue, []byte{1}) }, ble.OperationNotSupported},
	} {
		if err := check.call(); !errors.Is(err, check.code) {
			t.Fatalf("want %v, got %v", check.code, err)
		}
	}
	if _, err := p.Services(); err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(advertised[0].Data, []byte{0, 0x7f, 0x80, 0xff}) || string(manufacturer[0x004c]) != "test" || value.Descriptors[1].UUID != description {
		t.Fatal("native calls overwrote retained Go metadata")
	}
	if err := p.Disconnect(); err != nil {
		t.Fatal(err)
	}
	if _, err := p.Read(testService, testValue); !errors.Is(err, ble.PeripheralNotConnected) {
		t.Fatal(err)
	}
}

func TestPlainSubscriptionsAndConnectionEvents(t *testing.T) {
	a := plainAdapter(t)
	p := plainPeripheral(t, a)
	events := make(chan string, 8)
	if err := p.SetCallbackOnConnected(func() { events <- "connected" }); err != nil {
		t.Fatal(err)
	}
	if err := p.SetCallbackOnDisconnected(func() { events <- "disconnected" }); err != nil {
		t.Fatal(err)
	}
	if err := p.Connect(); err != nil {
		t.Fatal(err)
	}
	if event := receive(t, events); event != "connected" {
		t.Fatal(event)
	}
	paired, err := a.GetPairedPeripherals()
	if err != nil || len(paired) != 1 {
		t.Fatalf("paired devices = %d, %v", len(paired), err)
	}
	for _, peer := range paired {
		defer peer.Close()
	}
	connected, err := a.GetConnectedPeripherals()
	if err != nil || len(connected) != 1 {
		t.Fatalf("connected devices = %d, %v", len(connected), err)
	}
	for _, peer := range connected {
		defer peer.Close()
	}
	if paired, err := p.IsPaired(); err != nil || !paired {
		t.Fatalf("paired = %v, %v", paired, err)
	}
	for i, subscribe := range []func(string, string, func([]byte)) error{p.Notify, p.Indicate} {
		payload := []byte{0, 0xff, byte(i)}
		if err := p.WriteRequest(testService, testValue, payload); err != nil {
			t.Fatal(err)
		}
		values := make(chan []byte, 8)
		if err := subscribe("FFF0", "FFF1", func(data []byte) { values <- data }); err != nil {
			t.Fatal(err)
		}
		if flags, err := p.ReadDescriptor(testService, testValue, cccd); err != nil || !bytes.Equal(flags, []byte{byte(i + 1), 0}) {
			t.Fatalf("subscription flags = %v, %v", flags, err)
		}
		retained := receive(t, values)
		if !bytes.Equal(retained, payload) {
			t.Fatalf("subscription value = %v", retained)
		}
		if err := p.WriteCommand(testService, testValue, []byte{42}); err != nil {
			t.Fatal(err)
		}
		if data := receive(t, values); !bytes.Equal(data, []byte{42}) || !bytes.Equal(retained, payload) {
			t.Fatalf("callback buffers were reused: %v, retained %v", data, retained)
		}
		if err := p.Unsubscribe(testService, testValue); err != nil {
			t.Fatal(err)
		}
		if flags, err := p.ReadDescriptor("fff0", "fff1", "2902"); err != nil || !bytes.Equal(flags, []byte{0, 0}) {
			t.Fatalf("unsubscribe flags = %v, %v", flags, err)
		}
	}
	if err := p.Disconnect(); err != nil {
		t.Fatal(err)
	}
	if event := receive(t, events); event != "disconnected" {
		t.Fatal(event)
	}
	if err := p.Unpair(); err != nil {
		t.Fatal(err)
	}
	if paired, err := p.IsPaired(); err != nil || paired {
		t.Fatalf("unpair = %v, %v", paired, err)
	}
}
