package simplegoble

// AddressType describes the peripheral's Bluetooth address.
type AddressType uint32

const (
	AddressPublic AddressType = iota
	AddressRandom
	AddressUnspecified
)

// Service contains advertised data or discovered GATT characteristics.
// Its data is owned by Go and remains valid after the peripheral is closed.
type Service struct {
	UUID            string
	Data            []byte
	Characteristics []Characteristic
}

// Characteristic describes the operations supported by a GATT characteristic.
type Characteristic struct {
	UUID                                                              string
	CanRead, CanWriteRequest, CanWriteCommand, CanNotify, CanIndicate bool
	Descriptors                                                       []Descriptor
}

// Descriptor identifies a GATT characteristic descriptor.
type Descriptor struct {
	UUID string
}
