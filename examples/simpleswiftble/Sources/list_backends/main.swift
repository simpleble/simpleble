import Foundation
import SimpleSwiftBLE

for backend in try Backend.getBackends() {
    print("Backend: \(backend.identifier)")
    for adapter in try backend.getAdapters() {
        print("    Adapter: \(adapter.identifier) [\(adapter.address)]")
    }
}
