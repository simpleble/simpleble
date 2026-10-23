internal import CSimpleBLE
import Dispatch
import Foundation

/// A SimpleCBLE handle. SimpleCBLE handles can be used from any thread.
struct NativeHandle: @unchecked Sendable {
    let raw: UnsafeMutableRawPointer
}

/// A value guarded by a lock.
final class Locked<Value>: @unchecked Sendable {
    private let lock = NSLock()
    private var value: Value

    init(_ value: Value) {
        self.value = value
    }

    func withLock<T>(_ body: (inout Value) throws -> T) rethrows -> T {
        lock.lock()
        defer { lock.unlock() }
        return try body(&value)
    }
}

/// Calls a SimpleCBLE function and throws the error it reports.
func check<T>(_ body: (UnsafeMutablePointer<OpaquePointer?>) -> T) throws -> T {
    var error: OpaquePointer?
    let result = body(&error)
    if let error {
        throw SimpleBLEError(taking: error)
    }
    return result
}

/// Calls a SimpleCBLE function that reads state the library already holds. Properties don't throw, so a failure
/// returns the value SimpleCBLE reports for it.
func known<T>(_ body: (UnsafeMutablePointer<OpaquePointer?>) -> T) -> T {
    var error: OpaquePointer?
    defer { simpleble_error_release(&error) }
    return body(&error)
}

/// Copies and frees a string that SimpleCBLE allocated.
func takeString(_ raw: UnsafeMutablePointer<CChar>?) -> String {
    guard let raw else { return "" }
    defer { simpleble_free(raw) }
    return String(cString: raw)
}

/// The key under which callbacks find the Swift object of a native one.
func identityKey(_ identity: UnsafeRawPointer?) -> UInt {
    UInt(bitPattern: identity)
}

let blockingQueue = DispatchQueue(label: "org.simpleble.simpleswiftble", attributes: .concurrent)

/// Runs a call that waits on the Bluetooth stack outside Swift's cooperative thread pool.
func blocking<T: Sendable>(_ body: @escaping @Sendable () throws -> T) async throws -> T {
    try await withCheckedThrowingContinuation { continuation in
        blockingQueue.async {
            continuation.resume(with: Result { try body() })
        }
    }
}

/// A UUID in SimpleCBLE's fixed-size form.
func nativeUUID(_ uuid: BluetoothUUID) -> simpleble_uuid_t {
    var result = simpleble_uuid_t()
    withUnsafeMutableBytes(of: &result.value) { buffer in
        buffer.copyBytes(from: uuid.value.utf8.prefix(buffer.count - 1))
    }
    return result
}

/// A UUID read from SimpleCBLE's fixed-size form.
func bluetoothUUID(_ uuid: simpleble_uuid_t) -> BluetoothUUID {
    var value = uuid.value
    return withUnsafeBytes(of: &value) { buffer in
        BluetoothUUID(String(decoding: buffer.prefix { $0 != 0 }, as: UTF8.self))
    }
}

/// Copies a buffer that SimpleCBLE owns.
func data(_ bytes: UnsafePointer<UInt8>?, count: Int) -> Data {
    guard let bytes, count > 0 else { return Data() }
    return Data(bytes: bytes, count: count)
}

/// Copies and frees a buffer that SimpleCBLE allocated.
func takeData(_ bytes: UnsafeMutablePointer<UInt8>?, count: Int) -> Data {
    defer { simpleble_free(bytes) }
    return data(bytes, count: count)
}

/// Reads a SimpleCBLE peripheral list into handles. The caller owns them.
func readPeripheralHandles(
    _ read: (UnsafeMutablePointer<simpleble_peripheral_list_t?>, UnsafeMutablePointer<OpaquePointer?>) -> Int
) throws -> [NativeHandle] {
    var list: simpleble_peripheral_list_t?
    let count = try check { read(&list, $0) }
    defer { simpleble_peripheral_list_release(list) }
    var handles: [NativeHandle] = []
    do {
        for index in 0..<count {
            handles.append(NativeHandle(raw: try check { simpleble_peripheral_list_get(list, index, $0) }!))
        }
    } catch {
        handles.forEach { simpleble_peripheral_release_handle($0.raw) }
        throw error
    }
    return handles
}
