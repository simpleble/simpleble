import Foundation
import SimpleSwiftBLE
import Testing

extension SimpleSwiftBLETests {
    @Suite struct GattTests {
        @Test func readWriteAndNotifyUntilDisconnection() async throws {
            let peripheral = try await Plain.peripheral()
            try await peripheral.connect()

            let service = try #require(try peripheral.services().first { $0.uuid == Plain.testService })
            let characteristic = try #require(service.characteristics.first { $0.uuid == Plain.binaryValue })
            #expect(characteristic.canNotify && characteristic.descriptors.contains { $0.uuid == Plain.userDescription })

            let initial = try await peripheral.read(service: Plain.testService, characteristic: Plain.binaryValue)
            #expect(initial == Data([0x00, 0x7f, 0x80, 0xff]))
            try await peripheral.writeRequest(service: Plain.testService, characteristic: Plain.binaryValue, data: Data([1, 2]))
            #expect(try await peripheral.read(service: Plain.testService, characteristic: Plain.binaryValue) == Data([1, 2]))
            try await peripheral.writeRequest(service: Plain.testService, characteristic: Plain.binaryValue, data: initial)

            var values = try await peripheral.notify(service: Plain.testService, characteristic: Plain.binaryValue)
                .makeAsyncIterator()
            #expect(await values.next() == initial)

            let disconnected = peripheral.onDisconnected
            try await peripheral.disconnect()
            while await values.next() != nil {}
            #expect(await disconnected.first { true } != nil)
        }
    }
}
