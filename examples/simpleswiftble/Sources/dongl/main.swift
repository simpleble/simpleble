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

let adapters = try Backend.getDongl().getAdapters()
guard !adapters.isEmpty else {
    print("No Dongl found")
    exit(1)
}

let adapter = choose("a Dongl", adapters, describe)
print("Selected Dongl: \(describe(adapter))")

let found = adapter.onScanFound
let printer = Task { for await peripheral in found { print("Found \(describe(peripheral))") } }

// Scan for 5 seconds
try await adapter.scanFor(timeoutMs: 5000)
printer.cancel()

let peripherals = try adapter.scanGetResults()
guard !peripherals.isEmpty else {
    print("No peripherals found")
    exit(1)
}
let peripheral = choose("a peripheral", peripherals, describe)

// Pairing can start as soon as the link is up, so set the handlers before connecting. The Dongl pairs without the
// operating system.
try peripheral.setPasskeyRequestHandler {
    print("Enter the passkey shown on the device (empty to reject): ", terminator: "")
    let passkey = readLine() ?? ""
    return passkey.isEmpty ? nil : passkey
}
try peripheral.setPasskeyDisplayHandler { passkey in
    print("Enter \(passkey) on the device.")
}
try peripheral.setNumericComparisonHandler { passkey in
    print("Does the device show \(passkey)? [y/N] ", terminator: "")
    return readLine() == "y"
}

print("Connecting to: \(describe(peripheral))")
try await peripheral.connect()

print("Successfully connected, listing services...")
for service in try peripheral.services() {
    print("Service: \(service.uuid)")
    for characteristic in service.characteristics {
        print("    Characteristic: \(characteristic.uuid)")
    }
}
print("Paired: \(try await peripheral.isPaired())")

try await peripheral.disconnect()
