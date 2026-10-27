import Foundation
import SimpleSwiftBLE
import Testing

extension SimpleSwiftBLETests {
    @Suite struct ScanTests {
        @Test func scanEventsAndResultsShareObjects() async throws {
            let adapter = try #require(try Adapter.getAdapters().first)
            #expect(try Adapter.getAdapters().first === adapter)

            let started = adapter.onScanStart
            let stopped = adapter.onScanStop
            let found = adapter.onScanFound
            try await adapter.scanFor(timeoutMs: 500)

            #expect(await started.first { true } != nil)
            #expect(await stopped.first { true } != nil)
            let peripheral = try #require(await found.first { $0.identifier == "Plain Peripheral" })
            #expect(try adapter.scanGetResults().contains { $0 === peripheral })
            #expect(try peripheral.manufacturerData() == [0x004c: Data("test".utf8)])
        }

        @Test func failuresCarryTheErrorCode() async throws {
            let adapter = try #require(try Adapter.getAdapters().first)
            do {
                try await adapter.scanFor(timeoutMs: -1)
                Issue.record("scanFor accepted a negative duration")
            } catch let error as SimpleBLEError {
                #expect(error.code == .invalidArgument)
            }
        }
    }
}
