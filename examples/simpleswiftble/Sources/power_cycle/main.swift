import Foundation
import SimpleSwiftBLE

let adapters = try Adapter.getAdapters()
if adapters.isEmpty {
    print("No adapters found")
}

for adapter in adapters {
    print("Adapter: \(adapter.identifier) [\(adapter.address)]")

    print("Powering off adapter")
    try await adapter.powerOff()
    print("Adapter powered: \(try await adapter.isPowered())")
    try await Task.sleep(for: .seconds(1))

    print("Powering on adapter")
    try await adapter.powerOn()
    print("Adapter powered: \(try await adapter.isPowered())")
}
