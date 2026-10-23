internal import CSimpleBLE
import Foundation

/// Every adapter seen so far, by identity, so each one has a single Swift object.
private let adapters = Locked<[UInt: Adapter]>([:])

/// A Bluetooth adapter. Every enumeration returns the same object for an adapter.
public final class Adapter: Sendable {
    let handle: NativeHandle
    private let key: UInt

    private let powerOnEvents = EventSource<Void>()
    private let powerOffEvents = EventSource<Void>()
    private let scanStartEvents = EventSource<Void>()
    private let scanStopEvents = EventSource<Void>()
    private let scanUpdatedEvents = EventSource<Peripheral>()
    private let scanFoundEvents = EventSource<Peripheral>()

    private init(handle: NativeHandle, key: UInt) {
        self.handle = handle
        self.key = key
    }

    public static func isBluetoothEnabled() throws -> Bool {
        try check { simpleble_adapter_is_bluetooth_enabled($0) }
    }

    public static func getAdapters() throws -> [Adapter] {
        var list: simpleble_adapter_list_t?
        let count = try check { simpleble_adapter_get_adapters(&list, $0) }
        defer { simpleble_adapter_list_release(list) }
        return try (0..<count).map { index in
            let raw = try check { simpleble_adapter_list_get(list, index, $0) }
            return intern(raw!)
        }
    }

    public var identifier: String {
        takeString(known { simpleble_adapter_identifier(handle.raw, $0) })
    }

    public var address: BluetoothAddress {
        BluetoothAddress(takeString(known { simpleble_adapter_address(handle.raw, $0) }))
    }

    /// Asks the Bluetooth stack whether this adapter is on.
    public func isPowered() async throws -> Bool {
        try await blocking { [handle] in try check { simpleble_adapter_is_powered(handle.raw, $0) } }
    }

    /// Switches the adapter on. iOS and Mac Catalyst leave that to the user, so there this changes nothing.
    public func powerOn() async throws {
        try await blocking { [handle] in try check { simpleble_adapter_power_on(handle.raw, $0) } }
    }

    /// Switches the adapter off. iOS and Mac Catalyst leave that to the user, so there this changes nothing.
    public func powerOff() async throws {
        try await blocking { [handle] in try check { simpleble_adapter_power_off(handle.raw, $0) } }
    }

    public var onPowerOn: AsyncStream<Void> { powerOnEvents.stream() }

    public var onPowerOff: AsyncStream<Void> { powerOffEvents.stream() }

    public func scanStart() throws {
        try check { simpleble_adapter_scan_start(handle.raw, $0) }
    }

    public func scanStop() throws {
        try check { simpleble_adapter_scan_stop(handle.raw, $0) }
    }

    /// Scans for that many milliseconds and returns when the scan ends.
    public func scanFor(timeoutMs: Int) async throws {
        try await blocking { [handle] in
            try check { simpleble_adapter_scan_for(handle.raw, Int32(clamping: timeoutMs), $0) }
        }
    }

    /// Asks the Bluetooth stack whether this adapter is scanning.
    public func scanIsActive() async throws -> Bool {
        try await blocking { [handle] in try check { simpleble_adapter_scan_is_active(handle.raw, $0) } }
    }

    /// The peripherals seen by the last scan.
    public func scanGetResults() throws -> [Peripheral] {
        try readPeripheralHandles { simpleble_adapter_scan_get_results(handle.raw, $0, $1) }.map { Peripheral.intern($0.raw) }
    }

    public var onScanStart: AsyncStream<Void> { scanStartEvents.stream() }

    public var onScanStop: AsyncStream<Void> { scanStopEvents.stream() }

    /// Emits a peripheral each time a scan sees it again.
    public var onScanUpdated: AsyncStream<Peripheral> { scanUpdatedEvents.stream() }

    /// Emits a peripheral the first time a scan sees it.
    public var onScanFound: AsyncStream<Peripheral> { scanFoundEvents.stream() }

    /// Asks the Bluetooth stack for the peripherals paired with this adapter.
    public func getPairedPeripherals() async throws -> [Peripheral] {
        let handles = try await blocking { [handle] in
            try readPeripheralHandles { simpleble_adapter_get_paired_peripherals(handle.raw, $0, $1) }
        }
        return handles.map { Peripheral.intern($0.raw) }
    }

    /// Asks the Bluetooth stack for the peripherals connected to this adapter.
    public func getConnectedPeripherals() async throws -> [Peripheral] {
        let handles = try await blocking { [handle] in
            try readPeripheralHandles { simpleble_adapter_get_connected_peripherals(handle.raw, $0, $1) }
        }
        return handles.map { Peripheral.intern($0.raw) }
    }

    /// Returns the peripherals CoreBluetooth already knows, by the `address` values they had before, without
    /// scanning. Unknown addresses are skipped. The adapter must be powered on.
    public func retrieveCachedPeripherals(_ addresses: [BluetoothAddress]) throws -> [Peripheral] {
        try addresses.compactMap { address in
            #if os(macOS)
                let raw = try check { simpleble_advanced_macos_retrieve_cached_peripheral(handle.raw, address.value, $0) }
            #else
                let raw = try check { simpleble_advanced_ios_retrieve_cached_peripheral(handle.raw, address.value, $0) }
            #endif
            return raw.map(Peripheral.intern)
        }
    }

    /// Creates a local peripheral that hosts GATT services on this adapter.
    public func createLocalPeripheral() throws -> LocalPeripheral {
        LocalPeripheral.create(try check { simpleble_adapter_create_local_peripheral(handle.raw, $0) }!)
    }

    /// Returns the adapter's object, keeping the first handle to it and releasing later ones.
    static func intern(_ raw: UnsafeMutableRawPointer) -> Adapter {
        let key = identityKey(simpleble_adapter_identity(raw))
        let (adapter, created) = adapters.withLock { adapters in
            if let adapter = adapters[key] {
                return (adapter, false)
            }
            let adapter = Adapter(handle: NativeHandle(raw: raw), key: key)
            adapters[key] = adapter
            return (adapter, true)
        }
        guard created else {
            simpleble_adapter_release_handle(raw)
            return adapter
        }
        // NOTE: Callbacks are registered outside the lock, since SimpleBLE may hold its own locks while it calls them.
        adapter.registerCallbacks()
        return adapter
    }

    private static func find(_ userdata: UnsafeMutableRawPointer?) -> Adapter? {
        adapters.withLock { adapters in adapters[UInt(bitPattern: userdata)] }
    }

    private func registerCallbacks() {
        let userdata = UnsafeMutableRawPointer(bitPattern: key)
        simpleble_adapter_set_callback_on_power_on(
            handle.raw, { _, userdata in Adapter.find(userdata)?.powerOnEvents.yield(()) }, userdata)
        simpleble_adapter_set_callback_on_power_off(
            handle.raw, { _, userdata in Adapter.find(userdata)?.powerOffEvents.yield(()) }, userdata)
        simpleble_adapter_set_callback_on_scan_start(
            handle.raw, { _, userdata in Adapter.find(userdata)?.scanStartEvents.yield(()) }, userdata)
        simpleble_adapter_set_callback_on_scan_stop(
            handle.raw, { _, userdata in Adapter.find(userdata)?.scanStopEvents.yield(()) }, userdata)
        simpleble_adapter_set_callback_on_scan_updated(
            handle.raw,
            { _, raw, userdata in
                guard let raw else { return }
                guard let adapter = Adapter.find(userdata) else {
                    simpleble_peripheral_release_handle(raw)
                    return
                }
                adapter.scanUpdatedEvents.yield(Peripheral.intern(raw))
            }, userdata)
        simpleble_adapter_set_callback_on_scan_found(
            handle.raw,
            { _, raw, userdata in
                guard let raw else { return }
                guard let adapter = Adapter.find(userdata) else {
                    simpleble_peripheral_release_handle(raw)
                    return
                }
                adapter.scanFoundEvents.yield(Peripheral.intern(raw))
            }, userdata)
    }
}

extension Adapter: Hashable, Identifiable {
    public static func == (lhs: Adapter, rhs: Adapter) -> Bool {
        lhs === rhs
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(ObjectIdentifier(self))
    }
}
