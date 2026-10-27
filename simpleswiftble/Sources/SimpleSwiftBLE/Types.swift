internal import CSimpleBLE
import Foundation

/// A Bluetooth address. On iOS and macOS, CoreBluetooth hides addresses, so this is the UUID the system assigns to a
/// peripheral for your app.
public struct BluetoothAddress: Hashable, Sendable, ExpressibleByStringLiteral, CustomStringConvertible {
    public let value: String

    public init(_ value: String) {
        self.value = value
    }

    public init(stringLiteral value: String) {
        self.value = value
    }

    public var description: String { value }
}

/// A Bluetooth UUID, holding its full 128-bit string.
public struct BluetoothUUID: Hashable, Sendable, ExpressibleByStringLiteral, CustomStringConvertible {
    public let value: String

    public init(_ value: String) {
        self.value = value
    }

    public init(stringLiteral value: String) {
        self.value = value
    }

    public var description: String { value }
}

/// A GATT service. Before connecting, `services()` lists the advertised services and the data they advertised.
public struct Service: Hashable, Sendable {
    public let uuid: BluetoothUUID
    public let data: Data
    public let characteristics: [Characteristic]

    init(_ service: simpleble_service_t) {
        uuid = bluetoothUUID(service.uuid)
        data = SimpleSwiftBLE.data(service.data, count: service.data_length)
        characteristics = (0..<service.characteristic_count).map { Characteristic(service.characteristics[$0]) }
    }
}

public struct Characteristic: Hashable, Sendable {
    public let uuid: BluetoothUUID
    public let descriptors: [Descriptor]
    public let canRead: Bool
    public let canWriteRequest: Bool
    public let canWriteCommand: Bool
    public let canNotify: Bool
    public let canIndicate: Bool

    init(_ characteristic: simpleble_characteristic_t) {
        uuid = bluetoothUUID(characteristic.uuid)
        descriptors = (0..<characteristic.descriptor_count).map { Descriptor(uuid: bluetoothUUID(characteristic.descriptors[$0].uuid)) }
        canRead = characteristic.can_read
        canWriteRequest = characteristic.can_write_request
        canWriteCommand = characteristic.can_write_command
        canNotify = characteristic.can_notify
        canIndicate = characteristic.can_indicate
    }
}

public struct Descriptor: Hashable, Sendable {
    public let uuid: BluetoothUUID
}

public enum BluetoothAddressType: Sendable {
    case `public`
    case random
    case unspecified

    init(_ type: simpleble_address_type_t) {
        switch type {
        case SIMPLEBLE_ADDRESS_TYPE_PUBLIC: self = .public
        case SIMPLEBLE_ADDRESS_TYPE_RANDOM: self = .random
        default: self = .unspecified
        }
    }
}

/// The library itself.
public enum SimpleBLE {
    /// SimpleBLE's version.
    public static var version: String { String(cString: simpleble_get_version()) }
}
