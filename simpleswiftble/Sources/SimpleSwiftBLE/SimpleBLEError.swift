internal import CSimpleBLE
import Foundation

/// A failure reported by SimpleBLE.
public struct SimpleBLEError: Error, Sendable, LocalizedError {
    /// SimpleBLE's error codes.
    public enum Code: Int, Sendable {
        case invalidArgument = 0
        case outOfMemory = 1
        case objectNotInitialized = 2
        case invalidBackendReference = 3
        case peripheralNotConnected = 4
        case gattServiceNotFound = 5
        case gattCharacteristicNotFound = 6
        case gattDescriptorNotFound = 7
        case operationNotSupported = 8
        case operationFailed = 9
        case winrtAccessDenied = 10
        case winrtException = 11
        case coreBluetoothException = 12
        case unclassifiedException = 13
        case adapterInUse = 14
    }

    public let code: Code
    public let message: String

    public var errorDescription: String? { message }

    init(code: Code, message: String) {
        self.code = code
        self.message = message
    }

    /// Reads and releases an error that SimpleCBLE reported.
    init(taking error: OpaquePointer) {
        var error: OpaquePointer? = error
        code = Code(rawValue: Int(simpleble_error_code(error).rawValue)) ?? .unclassifiedException
        message = String(cString: simpleble_error_message(error))
        simpleble_error_release(&error)
    }
}
