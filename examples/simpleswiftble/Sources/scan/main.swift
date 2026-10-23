import Foundation
import SimpleSwiftBLE

/// Lists `items` and returns the one the user picks.
func choose<Item>(_ title: String, _ items: [Item], _ describe: (Item) -> String) -> Item {
    print("Please select \(title):")
    for (index, item) in items.enumerated() {
        print("\(index): \(describe(item))")
    }
    print("Enter choice: ", terminator: "")
    guard let choice = readLine().flatMap({ Int($0) }), items.indices.contains(choice) else {
        print("Invalid choice")
        exit(1)
    }
    return items[choice]
}

func describe(_ adapter: Adapter) -> String {
    "\(adapter.identifier) [\(adapter.address)]"
}

func describe(_ peripheral: Peripheral) -> String {
    "\(peripheral.identifier) [\(peripheral.address)]"
}

extension Data {
    var hex: String { map { String(format: "%02x", $0) }.joined() }
}

let adapters = try Adapter.getAdapters()
guard !adapters.isEmpty else {
    print("No adapters found")
    exit(1)
}

let adapter = choose("an adapter", adapters, describe)
print("Selected adapter: \(describe(adapter))")

let started = adapter.onScanStart
let found = adapter.onScanFound
let stopped = adapter.onScanStop
Task { for await _ in started { print("Scan started.") } }
let printer = Task { for await peripheral in found { print("Found \(describe(peripheral))") } }

// Scan for 5 seconds
try await adapter.scanFor(timeoutMs: 5000)
for await _ in stopped {
    print("Scan complete.")
    break
}
printer.cancel()

print("The following peripherals were found:")
for peripheral in try adapter.scanGetResults() {
    let connectable = peripheral.isConnectable ? "Connectable" : "Non-Connectable"
    print("\(describe(peripheral)) \(peripheral.rssi) dBm - \(connectable)")
    print("    Address Type: \(peripheral.addressType)")
    print("    Tx Power: \(peripheral.txPower) dBm")

    for (companyId, data) in try peripheral.manufacturerData() {
        print("    Manufacturer ID: \(companyId)")
        print("    Manufacturer data: \(data.hex)")
    }

    for service in try peripheral.services() {
        print("    Service UUID: \(service.uuid)")
        print("    Service data: \(service.data.hex)")
    }
}
