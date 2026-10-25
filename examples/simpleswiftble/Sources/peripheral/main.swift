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

let serviceUUID: BluetoothUUID = "12345678-1234-5678-1234-56789abcdef0"
let characteristicUUID: BluetoothUUID = "12345678-1234-5678-1234-56789abcdef1"

let adapters = try Adapter.getAdapters()
guard !adapters.isEmpty else {
    print("No adapters found")
    exit(1)
}
let adapter = choose("an adapter", adapters, describe)

let host = try adapter.createLocalPeripheral()

// Add a service UUID to the advertisement.
try host.addAdvertisedService(serviceUUID)

let service = try host.addService(serviceUUID)
let characteristic = try service.addCharacteristic(
    characteristicUUID, capabilities: [.read, .writeRequest, .writeCommand, .notify, .indicate])
try await characteristic.setValue(Data("ready".utf8))

let writes = characteristic.onWrite
let subscribed = characteristic.onSubscribed
let unsubscribed = characteristic.onUnsubscribed
let connected = host.onClientConnected
let disconnected = host.onClientDisconnected
Task {
    for await value in writes {
        print("Write: \(value.map { String(format: "%02x", $0) }.joined())")
        // Echo the written value to subscribed clients.
        do {
            try await characteristic.setValue(value)
        } catch {
            print("Echo failed: \(error.localizedDescription)")
        }
    }
}
Task { for await _ in subscribed { print("Client subscribed.") } }
Task { for await _ in unsubscribed { print("Client unsubscribed.") } }
Task { for await address in connected { print("Client connected: \(address)") } }
Task { for await address in disconnected { print("Client disconnected: \(address)") } }

try await host.start()
print("Advertising. Press Enter to stop.")

// Wait for Enter away from the main actor, which runs the tasks above.
await withCheckedContinuation { continuation in
    DispatchQueue.global().async {
        _ = readLine()
        continuation.resume()
    }
}
try await host.stop()
