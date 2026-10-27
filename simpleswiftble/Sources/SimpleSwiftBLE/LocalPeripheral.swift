internal import CSimpleBLE
import Foundation

// NOTE: Local GATT objects are kept for the whole process, as SimpleKtBLE does. Hosting ends with stop(), not when
// the last reference goes away.

/// Every local peripheral, by the number its callbacks receive.
private let localPeripherals = Locked<(nextID: UInt, objects: [UInt: LocalPeripheral])>((0, [:]))

/// Every local service and characteristic seen so far, by identity, so each one has a single Swift object.
private let localServices = Locked<[UInt: LocalService]>([:])
private let localCharacteristics = Locked<[UInt: LocalCharacteristic]>([:])

/// A GATT server hosted on an adapter. Add services and characteristics, then call `start()`.
public final class LocalPeripheral: Sendable {
    let handle: NativeHandle
    private let key: UInt
    private let clientConnectedEvents = EventSource<BluetoothAddress>()
    private let clientDisconnectedEvents = EventSource<BluetoothAddress>()

    private init(handle: NativeHandle, key: UInt) {
        self.handle = handle
        self.key = key
    }

    public func addAdvertisedService(_ uuid: BluetoothUUID) throws {
        try check { simpleble_local_peripheral_add_advertised_service(handle.raw, nativeUUID(uuid), $0) }
    }

    /// Adds manufacturer data to the advertisement. CoreBluetooth can't advertise it, so there this throws
    /// `operationNotSupported`.
    public func setAdvertisedManufacturerData(companyId: UInt16, data: Data) throws {
        try data.withUnsafeBytes { bytes in
            try check {
                simpleble_local_peripheral_set_advertised_manufacturer_data(
                    handle.raw, companyId, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, $0)
            }
        }
    }

    /// Advertises without accepting connections when false, as a beacon does. CoreBluetooth advertisements are always
    /// connectable, so there false throws `operationNotSupported`.
    public func setConnectable(_ connectable: Bool) throws {
        try check { simpleble_local_peripheral_set_connectable(handle.raw, connectable, $0) }
    }

    /// Sets the local name in the advertisement, or removes it with nil. This changes only the advertisement, not the
    /// system's Bluetooth name. macOS leaves it out when it doesn't fit beside the advertised service UUIDs, and iOS
    /// while the app is in the background.
    public func setAdvertisementLocalName(_ name: String?) throws {
        #if os(macOS)
            try check { simpleble_advanced_macos_set_advertisement_local_name(handle.raw, name, $0) }
        #elseif os(iOS)
            try check { simpleble_advanced_ios_set_advertisement_local_name(handle.raw, name, $0) }
        #endif
    }

    public func addService(_ uuid: BluetoothUUID) throws -> LocalService {
        LocalService.intern(try check { simpleble_local_peripheral_add_service(handle.raw, nativeUUID(uuid), $0) }!)
    }

    public var services: [LocalService] {
        var list: simpleble_local_service_list_t?
        let count = known { simpleble_local_peripheral_services(handle.raw, &list, $0) }
        defer { simpleble_local_service_list_release(list) }
        return (0..<count).compactMap { index in
            known { simpleble_local_service_list_get(list, index, $0) }.map(LocalService.intern)
        }
    }

    public func removeAllServices() throws {
        try check { simpleble_local_peripheral_remove_all_services(handle.raw, $0) }
    }

    /// Starts advertising and serving the services. Configure them before, and call `stop()` to change them.
    public func start() async throws {
        try await blocking { [handle] in try check { simpleble_local_peripheral_start(handle.raw, $0) } }
    }

    /// Stops advertising and disconnects the clients.
    public func stop() async throws {
        try await blocking { [handle] in try check { simpleble_local_peripheral_stop(handle.raw, $0) } }
    }

    public var isStarted: Bool {
        known { simpleble_local_peripheral_is_started(handle.raw, $0) }
    }

    /// Asks the Bluetooth stack whether the peripheral is advertising.
    public func isAdvertising() async throws -> Bool {
        try await blocking { [handle] in try check { simpleble_local_peripheral_is_advertising(handle.raw, $0) } }
    }

    /// Emits the address of each client that connects. CoreBluetooth reports no connections, so this follows a client's
    /// first subscription, and the address is the identifier CoreBluetooth assigns to the client.
    public var onClientConnected: AsyncStream<BluetoothAddress> { clientConnectedEvents.stream() }

    /// Emits the address of each client that disconnects. With CoreBluetooth, this follows a client's last
    /// unsubscription.
    public var onClientDisconnected: AsyncStream<BluetoothAddress> { clientDisconnectedEvents.stream() }

    static func create(_ raw: UnsafeMutableRawPointer) -> LocalPeripheral {
        let peripheral = localPeripherals.withLock { peripherals in
            peripherals.nextID += 1
            let peripheral = LocalPeripheral(handle: NativeHandle(raw: raw), key: peripherals.nextID)
            peripherals.objects[peripherals.nextID] = peripheral
            return peripheral
        }
        let userdata = UnsafeMutableRawPointer(bitPattern: peripheral.key)
        simpleble_local_peripheral_set_callback_on_client_connected(
            raw,
            { _, address, userdata in
                let address = BluetoothAddress(address.map { String(cString: $0) } ?? "")
                LocalPeripheral.find(userdata)?.clientConnectedEvents.yield(address)
            }, userdata)
        simpleble_local_peripheral_set_callback_on_client_disconnected(
            raw,
            { _, address, userdata in
                let address = BluetoothAddress(address.map { String(cString: $0) } ?? "")
                LocalPeripheral.find(userdata)?.clientDisconnectedEvents.yield(address)
            }, userdata)
        return peripheral
    }

    private static func find(_ userdata: UnsafeMutableRawPointer?) -> LocalPeripheral? {
        localPeripherals.withLock { $0.objects[UInt(bitPattern: userdata)] }
    }
}

/// A service of a local peripheral.
public final class LocalService: Sendable {
    let handle: NativeHandle
    public let uuid: BluetoothUUID

    private init(handle: NativeHandle) {
        self.handle = handle
        var uuid = simpleble_uuid_t()
        known { simpleble_local_service_uuid(handle.raw, &uuid, $0) }
        self.uuid = bluetoothUUID(uuid)
    }

    public var characteristics: [LocalCharacteristic] {
        var list: simpleble_local_characteristic_list_t?
        let count = known { simpleble_local_service_characteristics(handle.raw, &list, $0) }
        defer { simpleble_local_characteristic_list_release(list) }
        return (0..<count).compactMap { index in
            known { simpleble_local_characteristic_list_get(list, index, $0) }.map(LocalCharacteristic.intern)
        }
    }

    public func addCharacteristic(_ uuid: BluetoothUUID, capabilities: LocalCharacteristicCapabilities) throws
        -> LocalCharacteristic
    {
        LocalCharacteristic.intern(
            try check {
                simpleble_local_service_add_characteristic(handle.raw, nativeUUID(uuid), capabilities.rawValue, $0)
            }!)
    }

    /// Returns the service's object, keeping the first handle to it and releasing later ones.
    static func intern(_ raw: UnsafeMutableRawPointer) -> LocalService {
        let key = identityKey(simpleble_local_service_identity(raw))
        let (service, created) = localServices.withLock { services in
            if let service = services[key] {
                return (service, false)
            }
            let service = LocalService(handle: NativeHandle(raw: raw))
            services[key] = service
            return (service, true)
        }
        if !created {
            simpleble_local_service_release_handle(raw)
        }
        return service
    }
}

/// What a client can do with a local characteristic.
public struct LocalCharacteristicCapabilities: OptionSet, Hashable, Sendable {
    public let rawValue: UInt32

    public init(rawValue: UInt32) {
        self.rawValue = rawValue
    }

    public static let read = Self(rawValue: SIMPLEBLE_LOCAL_CHARACTERISTIC_READ.rawValue)
    public static let writeRequest = Self(rawValue: SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST.rawValue)
    public static let writeCommand = Self(rawValue: SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND.rawValue)
    public static let notify = Self(rawValue: SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY.rawValue)
    public static let indicate = Self(rawValue: SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE.rawValue)
}

/// The buffer a read handler's value is served from, which SimpleCBLE reads after the handler returns.
private struct ReadBuffer: @unchecked Sendable {
    let bytes: UnsafeMutablePointer<UInt8>
}

/// A characteristic of a local service.
public final class LocalCharacteristic: Sendable {
    let handle: NativeHandle
    private let key: UInt
    public let uuid: BluetoothUUID
    public let capabilities: LocalCharacteristicCapabilities
    private let readHandler = Locked<(@Sendable () -> Data)?>(nil)
    private let readBuffer = Locked<ReadBuffer?>(nil)
    private let writeEvents = EventSource<Data>()
    private let subscribedEvents = EventSource<Void>()
    private let unsubscribedEvents = EventSource<Void>()

    private init(handle: NativeHandle, key: UInt) {
        self.handle = handle
        self.key = key
        var uuid = simpleble_uuid_t()
        known { simpleble_local_characteristic_uuid(handle.raw, &uuid, $0) }
        self.uuid = bluetoothUUID(uuid)
        capabilities = LocalCharacteristicCapabilities(
            rawValue: known { simpleble_local_characteristic_capabilities(handle.raw, $0) })
    }

    /// The stored value.
    public var value: Data {
        var length = 0
        let bytes = known { simpleble_local_characteristic_value(handle.raw, &length, $0) }
        return takeData(bytes, count: length)
    }

    /// Updates the stored value and publishes the latest value to subscribed clients on a best-effort basis.
    public func setValue(_ value: Data) async throws {
        try await blocking { [handle] in
            try value.withUnsafeBytes { bytes in
                try check {
                    simpleble_local_characteristic_set_value(
                        handle.raw, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, $0)
                }
            }
        }
    }

    /// Answers each read with the handler's value instead of the stored one, until you pass nil. The handler runs on a
    /// SimpleBLE thread that waits for it, so it should return promptly.
    public func setReadHandler(_ handler: (@Sendable () -> Data)?) {
        readHandler.withLock { $0 = handler }
        simpleble_local_characteristic_set_callback_on_read(
            handle.raw,
            handler == nil
                ? nil
                : { _, length, userdata in
                    length?.pointee = 0
                    guard let characteristic = LocalCharacteristic.find(userdata) else { return nil }
                    return characteristic.serveRead(length)
                },
            UnsafeMutableRawPointer(bitPattern: key))
    }

    /// Emits each value a client writes.
    public var onWrite: AsyncStream<Data> { writeEvents.stream() }

    /// Emits when the first client subscribes.
    public var onSubscribed: AsyncStream<Void> { subscribedEvents.stream() }

    /// Emits when the last client unsubscribes.
    public var onUnsubscribed: AsyncStream<Void> { unsubscribedEvents.stream() }

    /// Returns the characteristic's object, keeping the first handle to it and releasing later ones.
    static func intern(_ raw: UnsafeMutableRawPointer) -> LocalCharacteristic {
        let key = identityKey(simpleble_local_characteristic_identity(raw))
        let (characteristic, created) = localCharacteristics.withLock { characteristics in
            if let characteristic = characteristics[key] {
                return (characteristic, false)
            }
            let characteristic = LocalCharacteristic(handle: NativeHandle(raw: raw), key: key)
            characteristics[key] = characteristic
            return (characteristic, true)
        }
        guard created else {
            simpleble_local_characteristic_release_handle(raw)
            return characteristic
        }
        // NOTE: Callbacks are registered outside the lock, since SimpleBLE may hold its own locks while it calls them.
        characteristic.registerCallbacks()
        return characteristic
    }

    private static func find(_ userdata: UnsafeMutableRawPointer?) -> LocalCharacteristic? {
        localCharacteristics.withLock { $0[UInt(bitPattern: userdata)] }
    }

    private func registerCallbacks() {
        let userdata = UnsafeMutableRawPointer(bitPattern: key)
        simpleble_local_characteristic_set_callback_on_write(
            handle.raw,
            { _, bytes, length, userdata in
                LocalCharacteristic.find(userdata)?.writeEvents.yield(data(bytes, count: length))
            }, userdata)
        simpleble_local_characteristic_set_callback_on_subscribed(
            handle.raw, { _, userdata in LocalCharacteristic.find(userdata)?.subscribedEvents.yield(()) }, userdata)
        simpleble_local_characteristic_set_callback_on_unsubscribed(
            handle.raw, { _, userdata in LocalCharacteristic.find(userdata)?.unsubscribedEvents.yield(()) }, userdata)
    }

    /// Copies the read handler's value into the buffer SimpleCBLE serves, which stays valid until the next read.
    private func serveRead(_ length: UnsafeMutablePointer<Int>?) -> UnsafePointer<UInt8>? {
        guard let handler = readHandler.withLock({ $0 }) else { return nil }
        let value = handler()
        let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: max(value.count, 1))
        value.copyBytes(to: buffer, count: value.count)
        readBuffer.withLock { previous in
            previous?.bytes.deallocate()
            previous = ReadBuffer(bytes: buffer)
        }
        length?.pointee = value.count
        return UnsafePointer(buffer)
    }
}
