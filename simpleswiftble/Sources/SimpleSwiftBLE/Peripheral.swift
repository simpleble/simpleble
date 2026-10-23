internal import CSimpleBLE
import Dispatch
import Foundation

/// Every peripheral seen so far, by identity, so each one has a single Swift object.
private let peripherals = Locked<[UInt: Peripheral]>([:])

/// The streams of active subscriptions, by the number their callbacks receive.
private let dataStreams = Locked<(nextID: UInt, continuations: [UInt: AsyncStream<Data>.Continuation])>((0, [:]))

private struct SubscriptionKey: Hashable {
    let service: BluetoothUUID
    let characteristic: BluetoothUUID
}

private struct PairingHandlers {
    var passkeyRequest: (@Sendable () async -> String?)?
    var passkeyDisplay: (@Sendable (String) async -> Void)?
    var numericComparison: (@Sendable (String) async -> Bool)?
}

/// A remote Bluetooth device. An adapter gives you the same object for a device, from a scan event or from a list.
public final class Peripheral: Sendable {
    let handle: NativeHandle
    private let key: UInt
    private let subscriptions = Locked<[SubscriptionKey: UInt]>([:])
    private let pairingHandlers = Locked(PairingHandlers())
    private let connectedEvents = EventSource<Void>()
    private let disconnectedEvents = EventSource<Void>()

    private init(handle: NativeHandle, key: UInt) {
        self.handle = handle
        self.key = key
    }

    public var identifier: String {
        takeString(known { simpleble_peripheral_identifier(handle.raw, $0) })
    }

    public var address: BluetoothAddress {
        BluetoothAddress(takeString(known { simpleble_peripheral_address(handle.raw, $0) }))
    }

    public var addressType: BluetoothAddressType {
        BluetoothAddressType(known { simpleble_peripheral_address_type(handle.raw, $0) })
    }

    public var rssi: Int {
        Int(known { simpleble_peripheral_rssi(handle.raw, $0) })
    }

    public var txPower: Int {
        Int(known { simpleble_peripheral_tx_power(handle.raw, $0) })
    }

    /// The largest payload a single write or notification can carry.
    public var mtu: Int {
        Int(known { simpleble_peripheral_mtu(handle.raw, $0) })
    }

    public var isConnected: Bool {
        known { simpleble_peripheral_is_connected(handle.raw, $0) }
    }

    public var isConnectable: Bool {
        known { simpleble_peripheral_is_connectable(handle.raw, $0) }
    }

    public func connect() async throws {
        try await blocking { [handle] in try check { simpleble_peripheral_connect(handle.raw, $0) } }
    }

    public func disconnect() async throws {
        try await blocking { [handle] in try check { simpleble_peripheral_disconnect(handle.raw, $0) } }
    }

    /// Asks the Bluetooth stack whether the device is paired.
    public func isPaired() async throws -> Bool {
        try await blocking { [handle] in try check { simpleble_peripheral_is_paired(handle.raw, $0) } }
    }

    public func unpair() async throws {
        try await blocking { [handle] in try check { simpleble_peripheral_unpair(handle.raw, $0) } }
    }

    /// The services, characteristics and descriptors of a connected peripheral. Before connecting, the services it
    /// advertises, each with the data it advertised.
    public func services() throws -> [Service] {
        var list: simpleble_service_list_t?
        let count = try check { simpleble_peripheral_services(handle.raw, &list, $0) }
        defer { simpleble_service_list_release(list) }
        return try (0..<count).map { index in
            var service = simpleble_service_t()
            try check { simpleble_service_list_get(list, index, &service, $0) }
            defer { simpleble_service_release(&service) }
            return Service(service)
        }
    }

    /// The manufacturer data in the latest advertisement, by company identifier.
    public func manufacturerData() throws -> [UInt16: Data] {
        var list: simpleble_manufacturer_data_list_t?
        let count = try check { simpleble_peripheral_manufacturer_data(handle.raw, &list, $0) }
        defer { simpleble_manufacturer_data_list_release(list) }
        var result: [UInt16: Data] = [:]
        for index in 0..<count {
            var entry = simpleble_manufacturer_data_t()
            try check { simpleble_manufacturer_data_list_get(list, index, &entry, $0) }
            result[entry.manufacturer_id] = data(entry.data, count: entry.data_length)
            simpleble_manufacturer_data_release(&entry)
        }
        return result
    }

    public func read(service: BluetoothUUID, characteristic: BluetoothUUID) async throws -> Data {
        try await blocking { [handle] in
            var length = 0
            let bytes = try check {
                simpleble_peripheral_read(handle.raw, nativeUUID(service), nativeUUID(characteristic), &length, $0)
            }
            return takeData(bytes, count: length)
        }
    }

    /// Writes a value and waits for the peripheral to confirm it.
    public func writeRequest(service: BluetoothUUID, characteristic: BluetoothUUID, data: Data) async throws {
        try await blocking { [handle] in
            try data.withUnsafeBytes { bytes in
                try check {
                    simpleble_peripheral_write_request(
                        handle.raw, nativeUUID(service), nativeUUID(characteristic),
                        bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, $0)
                }
            }
        }
    }

    /// Writes a value without waiting for the peripheral to confirm it.
    public func writeCommand(service: BluetoothUUID, characteristic: BluetoothUUID, data: Data) async throws {
        try await blocking { [handle] in
            try data.withUnsafeBytes { bytes in
                try check {
                    simpleble_peripheral_write_command(
                        handle.raw, nativeUUID(service), nativeUUID(characteristic),
                        bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, $0)
                }
            }
        }
    }

    /// Subscribes to a characteristic's notifications and returns once subscribed. The stream ends when the
    /// peripheral disconnects or `unsubscribe()` is called, and cancelling or dropping it unsubscribes.
    public func notify(service: BluetoothUUID, characteristic: BluetoothUUID) async throws -> AsyncStream<Data> {
        try await subscribe(service: service, characteristic: characteristic, indicate: false)
    }

    /// Subscribes to a characteristic's indications, like `notify()`.
    public func indicate(service: BluetoothUUID, characteristic: BluetoothUUID) async throws -> AsyncStream<Data> {
        try await subscribe(service: service, characteristic: characteristic, indicate: true)
    }

    /// Ends the subscription to a characteristic, and the stream that receives it.
    public func unsubscribe(service: BluetoothUUID, characteristic: BluetoothUUID) async throws {
        let id = subscriptions.withLock { $0.removeValue(forKey: SubscriptionKey(service: service, characteristic: characteristic)) }
        defer { if let id { Peripheral.takeStream(id)?.finish() } }
        try await blocking { [handle] in
            try check {
                simpleble_peripheral_unsubscribe(handle.raw, nativeUUID(service), nativeUUID(characteristic), $0)
            }
        }
    }

    public func read(service: BluetoothUUID, characteristic: BluetoothUUID, descriptor: BluetoothUUID) async throws
        -> Data
    {
        try await blocking { [handle] in
            var length = 0
            let bytes = try check {
                simpleble_peripheral_read_descriptor(
                    handle.raw, nativeUUID(service), nativeUUID(characteristic), nativeUUID(descriptor), &length, $0)
            }
            return takeData(bytes, count: length)
        }
    }

    public func write(service: BluetoothUUID, characteristic: BluetoothUUID, descriptor: BluetoothUUID, data: Data)
        async throws
    {
        try await blocking { [handle] in
            try data.withUnsafeBytes { bytes in
                try check {
                    simpleble_peripheral_write_descriptor(
                        handle.raw, nativeUUID(service), nativeUUID(characteristic), nativeUUID(descriptor),
                        bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count, $0)
                }
            }
        }
    }

    public var onConnected: AsyncStream<Void> { connectedEvents.stream() }

    public var onDisconnected: AsyncStream<Void> { disconnectedEvents.stream() }

    /// Answers a Dongl's request for the passkey the peer displays: six decimal digits, or nil to reject pairing.
    /// Pairing can begin as soon as the peripheral connects, so set it before `connect()`. Pass nil to remove it.
    public func setPasskeyRequestHandler(_ handler: (@Sendable () async -> String?)?) throws {
        pairingHandlers.withLock { $0.passkeyRequest = handler }
        try check {
            simpleble_advanced_dongl_set_passkey_request_callback(
                handle.raw,
                handler == nil
                    ? nil
                    : { _, passkey, userdata in
                        guard let passkey, let handler = Peripheral.find(userdata)?.pairingHandlers.withLock({ $0.passkeyRequest })
                        else { return false }
                        // SimpleBLE checks the digits and rejects pairing when this returns false.
                        guard let digits = waitFor(handler), digits.utf8.count == 6 else { return false }
                        for (index, digit) in digits.utf8.enumerated() {
                            passkey[index] = CChar(bitPattern: digit)
                        }
                        passkey[6] = 0
                        return true
                    },
                UnsafeMutableRawPointer(bitPattern: key), $0)
        }
    }

    /// Receives the six-digit passkey a Dongl asks the user to enter on the peer. Set it before `connect()`.
    public func setPasskeyDisplayHandler(_ handler: (@Sendable (String) async -> Void)?) throws {
        pairingHandlers.withLock { $0.passkeyDisplay = handler }
        try check {
            simpleble_advanced_dongl_set_passkey_display_callback(
                handle.raw,
                handler == nil
                    ? nil
                    : { _, passkey, userdata in
                        guard let handler = Peripheral.find(userdata)?.pairingHandlers.withLock({ $0.passkeyDisplay })
                        else { return }
                        let passkey = passkey.map { String(cString: $0) } ?? ""
                        waitFor { await handler(passkey) }
                    },
                UnsafeMutableRawPointer(bitPattern: key), $0)
        }
    }

    /// Confirms whether the six-digit passkey a Dongl shows matches the peer's. Return true to accept pairing.
    /// Set it before `connect()`.
    public func setNumericComparisonHandler(_ handler: (@Sendable (String) async -> Bool)?) throws {
        pairingHandlers.withLock { $0.numericComparison = handler }
        try check {
            simpleble_advanced_dongl_set_numeric_comparison_callback(
                handle.raw,
                handler == nil
                    ? nil
                    : { _, passkey, userdata in
                        guard let handler = Peripheral.find(userdata)?.pairingHandlers.withLock({ $0.numericComparison })
                        else { return false }
                        let passkey = passkey.map { String(cString: $0) } ?? ""
                        return waitFor { await handler(passkey) }
                    },
                UnsafeMutableRawPointer(bitPattern: key), $0)
        }
    }

    /// Secures a Dongl connection with LE Secure Connections. With `mitm`, the peer must also be authenticated
    /// through the passkey or numeric comparison handlers. Pairing started by this call creates no bond, and the
    /// Dongl disconnects if it fails.
    public func secureConnection(mitm: Bool = false) async throws {
        try await blocking { [handle] in try check { simpleble_advanced_dongl_secure_connection(handle.raw, mitm, $0) } }
    }

    /// Returns a peripheral's object, keeping the first handle to it and releasing later ones.
    static func intern(_ raw: UnsafeMutableRawPointer) -> Peripheral {
        let key = identityKey(simpleble_peripheral_identity(raw))
        let (peripheral, created) = peripherals.withLock { peripherals in
            if let peripheral = peripherals[key] {
                return (peripheral, false)
            }
            let peripheral = Peripheral(handle: NativeHandle(raw: raw), key: key)
            peripherals[key] = peripheral
            return (peripheral, true)
        }
        guard created else {
            simpleble_peripheral_release_handle(raw)
            return peripheral
        }
        // NOTE: Callbacks are registered outside the lock, since SimpleBLE may hold its own locks while it calls them.
        peripheral.registerCallbacks()
        return peripheral
    }

    private static func find(_ userdata: UnsafeMutableRawPointer?) -> Peripheral? {
        peripherals.withLock { peripherals in peripherals[UInt(bitPattern: userdata)] }
    }

    private static func takeStream(_ id: UInt) -> AsyncStream<Data>.Continuation? {
        dataStreams.withLock { $0.continuations.removeValue(forKey: id) }
    }

    private func registerCallbacks() {
        let userdata = UnsafeMutableRawPointer(bitPattern: key)
        simpleble_peripheral_set_callback_on_connected(
            handle.raw, { _, userdata in Peripheral.find(userdata)?.connectedEvents.yield(()) }, userdata)
        simpleble_peripheral_set_callback_on_disconnected(
            handle.raw,
            { _, userdata in
                guard let peripheral = Peripheral.find(userdata) else { return }
                // NOTE: The disconnection ends the subscriptions it had, after the values that arrived before it.
                let ids = peripheral.subscriptions.withLock { subscriptions in
                    defer { subscriptions.removeAll() }
                    return Array(subscriptions.values)
                }
                ids.forEach { Peripheral.takeStream($0)?.finish() }
                peripheral.disconnectedEvents.yield(())
            }, userdata)
    }

    private func subscribe(service: BluetoothUUID, characteristic: BluetoothUUID, indicate: Bool) async throws
        -> AsyncStream<Data>
    {
        let subscription = SubscriptionKey(service: service, characteristic: characteristic)
        let (stream, continuation) = AsyncStream.makeStream(of: Data.self, bufferingPolicy: .unbounded)
        let id = dataStreams.withLock { streams in
            streams.nextID += 1
            streams.continuations[streams.nextID] = continuation
            return streams.nextID
        }
        let claimed = subscriptions.withLock { subscriptions in
            guard subscriptions[subscription] == nil else { return false }
            subscriptions[subscription] = id
            return true
        }
        guard claimed else {
            Peripheral.takeStream(id)?.finish()
            let kind = indicate ? "An indication" : "A notification"
            throw SimpleBLEError(
                code: .operationFailed, message: "\(kind) stream is already active for characteristic \(characteristic)")
        }

        // Only a stream that was cancelled or dropped still owns its subscription. After a disconnection or
        // unsubscribe(), the next subscription may already be another stream's.
        continuation.onTermination = { [weak self, handle] _ in
            _ = Peripheral.takeStream(id)
            guard let self, self.release(subscription, id) else { return }
            blockingQueue.async {
                var error: OpaquePointer?
                simpleble_peripheral_unsubscribe(handle.raw, nativeUUID(service), nativeUUID(characteristic), &error)
                simpleble_error_release(&error)
            }
        }

        do {
            try await blocking { [handle] in
                let userdata = UnsafeMutableRawPointer(bitPattern: id)
                try check {
                    if indicate {
                        simpleble_peripheral_indicate(
                            handle.raw, nativeUUID(service), nativeUUID(characteristic),
                            { _, _, _, bytes, length, userdata in
                                Peripheral.stream(userdata)?.yield(data(bytes, count: length))
                            }, userdata, $0)
                    } else {
                        simpleble_peripheral_notify(
                            handle.raw, nativeUUID(service), nativeUUID(characteristic),
                            { _, _, _, bytes, length, userdata in
                                Peripheral.stream(userdata)?.yield(data(bytes, count: length))
                            }, userdata, $0)
                    }
                }
            }
        } catch {
            _ = release(subscription, id)
            continuation.finish()
            throw error
        }
        return stream
    }

    private static func stream(_ userdata: UnsafeMutableRawPointer?) -> AsyncStream<Data>.Continuation? {
        dataStreams.withLock { $0.continuations[UInt(bitPattern: userdata)] }
    }

    /// Gives up a subscription, and returns whether the stream `id` still had it.
    private func release(_ subscription: SubscriptionKey, _ id: UInt) -> Bool {
        subscriptions.withLock { subscriptions in
            guard subscriptions[subscription] == id else { return false }
            subscriptions[subscription] = nil
            return true
        }
    }
}

extension Peripheral: Hashable, Identifiable {
    public static func == (lhs: Peripheral, rhs: Peripheral) -> Bool {
        lhs === rhs
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(ObjectIdentifier(self))
    }
}

/// Runs an asynchronous handler from a SimpleBLE thread that waits for its answer.
func waitFor<T: Sendable>(_ body: @escaping @Sendable () async -> T) -> T {
    let result = Locked<T?>(nil)
    let done = DispatchSemaphore(value: 0)
    Task {
        let value = await body()
        result.withLock { $0 = value }
        done.signal()
    }
    done.wait()
    return result.withLock { $0! }
}
