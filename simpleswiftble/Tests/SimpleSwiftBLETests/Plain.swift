import SimpleSwiftBLE
import Testing

// NOTE: The tests run against SimpleBLE's PLAIN backend, whose state lasts for the whole process, so every suite
// is nested in this one and they run one at a time.
@Suite(.serialized)
struct SimpleSwiftBLETests {}

enum Plain {
    static let testService: BluetoothUUID = "0000fff0-0000-1000-8000-00805f9b34fb"
    static let binaryValue: BluetoothUUID = "0000fff1-0000-1000-8000-00805f9b34fb"
    static let userDescription: BluetoothUUID = "00002901-0000-1000-8000-00805f9b34fb"

    static func adapter() throws -> Adapter {
        try #require(try Adapter.getAdapters().first)
    }

    /// Scans until the named peripheral is found.
    static func peripheral(named name: String = "Plain Peripheral") async throws -> Peripheral {
        let adapter = try adapter()
        let found = adapter.onScanFound
        try adapter.scanStart()
        defer { try? adapter.scanStop() }
        return try #require(await found.first { $0.identifier == name })
    }
}
