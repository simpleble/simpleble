import Foundation
import SimpleSwiftBLE
import Testing

extension SimpleSwiftBLETests {
    @Suite struct HostingTests {
        @Test func clientsReachTheLocalCharacteristic() async throws {
            let serviceUUID: BluetoothUUID = "00001234-0000-1000-8000-00805f9b34fb"
            let valueUUID: BluetoothUUID = "00005678-0000-1000-8000-00805f9b34fb"
            let host = try Plain.adapter().createLocalPeripheral()
            let service = try host.addService(serviceUUID)
            let value = try service.addCharacteristic(valueUUID, capabilities: [.read, .writeRequest, .notify])
            try await value.setValue(Data([0x00, 0xff]))
            #expect(host.services.first === service && service.characteristics.first === value)

            let connected = host.onClientConnected
            let writes = value.onWrite
            try await host.start()
            let client = try await Plain.peripheral(named: "Plain Adapter Peripheral")
            try await client.connect()
            #expect(await connected.first { _ in true } != nil)

            #expect(try await client.read(service: serviceUUID, characteristic: valueUUID) == Data([0x00, 0xff]))
            value.setReadHandler { Data("dynamic".utf8) }
            #expect(try await client.read(service: serviceUUID, characteristic: valueUUID) == Data("dynamic".utf8))
            value.setReadHandler(nil)
            try await client.writeRequest(service: serviceUUID, characteristic: valueUUID, data: Data([7]))
            #expect(await writes.first { _ in true } == Data([7]))

            try await client.disconnect()
            try await host.stop()
        }
    }
}
