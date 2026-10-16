package simplegoble

import (
	"unsafe"

	"github.com/simpleble/simplegoble/internal/bindings"
)

// Peripheral is an owned remote BLE peripheral handle. Independent handles
// share the device's connection, values, subscriptions, and native callback slots.
type Peripheral struct {
	handle    *bindings.Handle
	callbacks *bindings.CallbackOwner
}

func newPeripheral(native *bindings.NativeAPI, nativeHandle uintptr) *Peripheral {
	callbacks := &bindings.CallbackOwner{}
	return &Peripheral{handle: bindings.NewPeripheral(native, nativeHandle, callbacks), callbacks: callbacks}
}

// Initialized reports whether the handle is open.
func (p *Peripheral) Initialized() bool { return p.handle.Initialized() }

// Close releases this wrapper and expires its callbacks. It waits for native
// calls, but not for a selected Go callback. It does not disconnect the device.
func (p *Peripheral) Close() error { return bindings.ClosePeripheral(p.handle, p.callbacks) }

// Identifier returns the device name.
func (p *Peripheral) Identifier() (string, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		return native.TakeString(native.PeripheralIdentifier(nativeHandle, nativeError))
	})
}

// Address returns the Bluetooth address or backend-specific identifier.
func (p *Peripheral) Address() (string, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) string {
		return native.TakeString(native.PeripheralAddress(nativeHandle, nativeError))
	})
}

// AddressType returns the reported Bluetooth address type.
func (p *Peripheral) AddressType() (AddressType, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) AddressType {
		return AddressType(native.PeripheralAddressType(nativeHandle, nativeError))
	})
}

// RSSI returns the received signal strength in dBm, or -32768 when unavailable.
func (p *Peripheral) RSSI() (int16, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) int16 {
		return native.PeripheralRSSI(nativeHandle, nativeError)
	})
}

// TxPower returns advertised transmit power in dBm, or -32768 when unavailable.
func (p *Peripheral) TxPower() (int16, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) int16 {
		return native.PeripheralTxPower(nativeHandle, nativeError)
	})
}

// MTU returns the backend's MTU in bytes.
func (p *Peripheral) MTU() (uint16, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) uint16 {
		return native.PeripheralMTU(nativeHandle, nativeError)
	})
}

// Connect connects to the peripheral and discovers its GATT services.
func (p *Peripheral) Connect() error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.PeripheralConnect(nativeHandle, nativeError)
	})
}

// Disconnect disconnects the peripheral, affecting all handles for that device.
func (p *Peripheral) Disconnect() error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.PeripheralDisconnect(nativeHandle, nativeError)
	})
}

// IsConnected reports the connection state.
func (p *Peripheral) IsConnected() (bool, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.PeripheralConnected(nativeHandle, nativeError)
	})
}

// IsConnectable reports whether the peripheral can be connected.
func (p *Peripheral) IsConnectable() (bool, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.PeripheralConnectable(nativeHandle, nativeError)
	})
}

// IsPaired reports whether the peripheral is paired.
func (p *Peripheral) IsPaired() (bool, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) bool {
		return native.PeripheralPaired(nativeHandle, nativeError)
	})
}

// Unpair removes the pairing with the device.
func (p *Peripheral) Unpair() error {
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		native.PeripheralUnpair(nativeHandle, nativeError)
	})
}

// Services returns advertised services when disconnected, or discovered GATT
// services when connected. All returned strings, bytes, and metadata are Go-owned.
func (p *Peripheral) Services() ([]Service, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []Service {
		count := native.PeripheralServiceCount(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		result := make([]Service, 0, count)
		for i := uintptr(0); i < count; i++ {
			var value bindings.Service
			native.PeripheralServiceGet(nativeHandle, i, &value, nativeError)
			if *nativeError != 0 {
				native.ServiceRelease(&value)
				return nil
			}
			service := Service{UUID: value.UUID.String(), Data: bindings.CopyBytes(value.Data, value.DataLength)}
			service.Characteristics = make([]Characteristic, 0, value.CharacteristicCount)
			for _, c := range unsafe.Slice(value.Characteristics, value.CharacteristicCount) {
				characteristic := Characteristic{UUID: c.UUID.String(), CanRead: c.CanRead, CanWriteRequest: c.CanWriteRequest,
					CanWriteCommand: c.CanWriteCommand, CanNotify: c.CanNotify, CanIndicate: c.CanIndicate}
				characteristic.Descriptors = make([]Descriptor, 0, c.DescriptorCount)
				for _, d := range unsafe.Slice(c.Descriptors, c.DescriptorCount) {
					characteristic.Descriptors = append(characteristic.Descriptors, Descriptor{UUID: d.UUID.String()})
				}
				service.Characteristics = append(service.Characteristics, characteristic)
			}
			native.ServiceRelease(&value)
			result = append(result, service)
		}
		return result
	})
}

// ManufacturerData returns Go-owned advertisement payloads indexed by company ID.
func (p *Peripheral) ManufacturerData() (map[uint16][]byte, error) {
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) map[uint16][]byte {
		count := native.PeripheralManufacturerCount(nativeHandle, nativeError)
		if *nativeError != 0 {
			return nil
		}
		result := make(map[uint16][]byte, count)
		for i := uintptr(0); i < count; i++ {
			var value bindings.ManufacturerData
			native.PeripheralManufacturerGet(nativeHandle, i, &value, nativeError)
			if *nativeError != 0 {
				native.ManufacturerRelease(&value)
				return nil
			}
			result[value.ID] = bindings.CopyBytes(value.Data, value.DataLength)
			native.ManufacturerRelease(&value)
		}
		return result
	})
}

// Read reads a characteristic value. UUIDs are canonical 128-bit UUIDs. The
// returned buffer is Go-owned, including empty values.
func (p *Peripheral) Read(service, characteristic string) ([]byte, error) {
	return p.read(service, characteristic, "")
}

// ReadDescriptor reads a GATT descriptor value.
func (p *Peripheral) ReadDescriptor(service, characteristic, descriptor string) ([]byte, error) {
	if descriptor == "" {
		return nil, &Error{Code: InvalidArgument, Message: "descriptor UUID is required"}
	}
	return p.read(service, characteristic, descriptor)
}

func (p *Peripheral) read(service, characteristic, descriptor string) ([]byte, error) {
	s, err := bindings.NewUUID(service)
	if err != nil {
		return nil, err
	}
	c, err := bindings.NewUUID(characteristic)
	if err != nil {
		return nil, err
	}
	var d bindings.UUID
	if descriptor != "" {
		d, err = bindings.NewUUID(descriptor)
		if err != nil {
			return nil, err
		}
	}
	return bindings.Query(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) []byte {
		var length uintptr
		var data *byte
		if descriptor == "" {
			data = native.PeripheralRead(nativeHandle, s, c, &length, nativeError)
		} else {
			data = native.PeripheralReadDescriptor(nativeHandle, s, c, d, &length, nativeError)
		}
		defer native.Free(unsafe.Pointer(data))
		if *nativeError != 0 {
			return nil
		}
		return bindings.CopyBytes(data, length)
	})
}

// WriteRequest writes a characteristic value with a response. Keep data
// unchanged until this method returns. Empty values are supported.
func (p *Peripheral) WriteRequest(service, characteristic string, data []byte) error {
	return p.write(service, characteristic, "", data, true)
}

// WriteCommand writes without a response. Keep data unchanged until it returns.
func (p *Peripheral) WriteCommand(service, characteristic string, data []byte) error {
	return p.write(service, characteristic, "", data, false)
}

// WriteDescriptor writes a descriptor value. Keep data unchanged until it returns.
func (p *Peripheral) WriteDescriptor(service, characteristic, descriptor string, data []byte) error {
	if descriptor == "" {
		return &Error{Code: InvalidArgument, Message: "descriptor UUID is required"}
	}
	return p.write(service, characteristic, descriptor, data, true)
}

func (p *Peripheral) write(service, characteristic, descriptor string, data []byte, request bool) error {
	s, err := bindings.NewUUID(service)
	if err != nil {
		return err
	}
	c, err := bindings.NewUUID(characteristic)
	if err != nil {
		return err
	}
	var d bindings.UUID
	if descriptor != "" {
		d, err = bindings.NewUUID(descriptor)
		if err != nil {
			return err
		}
	}
	return bindings.Command(p.handle, func(native *bindings.NativeAPI, nativeHandle uintptr, nativeError *uintptr) {
		input := bindings.InputBytes(data)
		if descriptor != "" {
			native.PeripheralWriteDescriptor(nativeHandle, s, c, d, input, uintptr(len(data)), nativeError)
		} else if request {
			native.PeripheralWriteRequest(nativeHandle, s, c, input, uintptr(len(data)), nativeError)
		} else {
			native.PeripheralWriteCommand(nativeHandle, s, c, input, uintptr(len(data)), nativeError)
		}
	})
}

// SetCallbackOnConnected replaces the connected callback; nil clears it.
// Callbacks follow the adapter dispatch rules and may call methods or Close.
func (p *Peripheral) SetCallbackOnConnected(fn func()) error {
	return bindings.SetCallback(p.handle, p.callbacks, bindings.Connected, fn)
}

// SetCallbackOnDisconnected replaces the disconnected callback; nil clears it.
func (p *Peripheral) SetCallbackOnDisconnected(fn func()) error {
	return bindings.SetCallback(p.handle, p.callbacks, bindings.Disconnected, fn)
}

// Notify subscribes to notifications. Each callback receives a Go-owned value
// that it can retain. It may call peripheral methods or Close. fn must be non-nil.
func (p *Peripheral) Notify(service, characteristic string, fn func([]byte)) error {
	return p.subscribe(service, characteristic, fn, false)
}

// Indicate subscribes to indications with the same dispatch rules as Notify.
func (p *Peripheral) Indicate(service, characteristic string, fn func([]byte)) error {
	return p.subscribe(service, characteristic, fn, true)
}

func (p *Peripheral) subscribe(service, characteristic string, fn func([]byte), indicate bool) error {
	if fn == nil {
		return &Error{Code: InvalidArgument, Message: "subscription callback is required"}
	}
	s, err := bindings.NewUUID(service)
	if err != nil {
		return err
	}
	c, err := bindings.NewUUID(characteristic)
	if err != nil {
		return err
	}
	return bindings.SetDataCallback(p.handle, p.callbacks, s, c, fn, indicate)
}

// Unsubscribe stops the native subscription, including one registered through
// another wrapper of the same device.
func (p *Peripheral) Unsubscribe(service, characteristic string) error {
	s, err := bindings.NewUUID(service)
	if err != nil {
		return err
	}
	c, err := bindings.NewUUID(characteristic)
	if err != nil {
		return err
	}
	return bindings.SetDataCallback(p.handle, p.callbacks, s, c, nil, false)
}
