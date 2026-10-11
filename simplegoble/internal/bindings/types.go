package bindings

import (
	"encoding/hex"
	"strings"
	"structs"
	"unsafe"
)

type UUID struct {
	_     structs.HostLayout
	Value [37]byte
}

func NewUUID(value string) (UUID, error) {
	var uuid UUID
	switch len(value) {
	case 4:
		value = "0000" + value + "-0000-1000-8000-00805f9b34fb"
	case 8:
		value += "-0000-1000-8000-00805f9b34fb"
	case 36:
	default:
		return uuid, &Error{Code: InvalidArgument, Message: "expected a 4/8-digit Bluetooth UUID or a canonical 128-bit UUID"}
	}
	if value[8] != '-' || value[13] != '-' || value[18] != '-' || value[23] != '-' {
		return uuid, &Error{Code: InvalidArgument, Message: "invalid UUID"}
	}
	if data, err := hex.DecodeString(strings.ReplaceAll(value, "-", "")); err != nil || len(data) != 16 {
		return uuid, &Error{Code: InvalidArgument, Message: "invalid UUID"}
	}
	copy(uuid.Value[:], strings.ToLower(value))
	return uuid, nil
}

func (uuid UUID) String() string {
	for i, b := range uuid.Value {
		if b == 0 {
			return string(uuid.Value[:i])
		}
	}
	return string(uuid.Value[:])
}

type Descriptor struct {
	_    structs.HostLayout
	UUID UUID
}

type Characteristic struct {
	_                                                                 structs.HostLayout
	UUID                                                              UUID
	CanRead, CanWriteRequest, CanWriteCommand, CanNotify, CanIndicate bool
	DescriptorCount                                                   uintptr
	Descriptors                                                       *Descriptor
}

type Service struct {
	_                   structs.HostLayout
	UUID                UUID
	DataLength          uintptr
	Data                *byte
	CharacteristicCount uintptr
	Characteristics     *Characteristic
}

type ManufacturerData struct {
	_          structs.HostLayout
	ID         uint16
	DataLength uintptr
	Data       *byte
}

// Copy borrowed native bytes before their owner releases or reuses them.
func CopyBytes(data *byte, length uintptr) []byte {
	if length == 0 {
		return []byte{}
	}
	return append([]byte(nil), unsafe.Slice(data, length)...)
}

// Empty writes still need a non-NULL pointer under the SimpleCBLE contract.
func InputBytes(data []byte) *byte {
	if len(data) == 0 {
		return new(byte)
	}
	return &data[0]
}
