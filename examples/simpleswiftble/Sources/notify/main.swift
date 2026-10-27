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

let peripherals = try adapter.scanGetResults()
guard !peripherals.isEmpty else {
    print("No peripherals found")
    exit(1)
}
let peripheral = choose("a peripheral", peripherals, describe)

print("Connecting to: \(describe(peripheral))")
try await peripheral.connect()

print("Successfully connected, listing services...")
let pairs = try peripheral.services().flatMap { service in
    service.characteristics.map { (service: service.uuid, characteristic: $0.uuid) }
}
guard !pairs.isEmpty else {
    print("No characteristics found")
    exit(1)
}
let pair = choose("a service/characteristic pair", pairs) { "\($0.service) \($0.characteristic)" }

let notifications = try await peripheral.notify(service: pair.service, characteristic: pair.characteristic)
let listener = Task {
    for await data in notifications {
        print("Notification: \(data.hex)")
    }
}
try await Task.sleep(for: .seconds(5))

try await peripheral.unsubscribe(service: pair.service, characteristic: pair.characteristic)
await listener.value

try await peripheral.disconnect()
