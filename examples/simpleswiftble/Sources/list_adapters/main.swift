import Foundation
import SimpleSwiftBLE

print("Bluetooth enabled: \(try Adapter.isBluetoothEnabled())")

let adapters = try Adapter.getAdapters()
if adapters.isEmpty {
    print("No adapters found")
}

for adapter in adapters {
    print("Adapter: \(adapter.identifier) [\(adapter.address)]")
    print("    Powered: \(try await adapter.isPowered())")
}
