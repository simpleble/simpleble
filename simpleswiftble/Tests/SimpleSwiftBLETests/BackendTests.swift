import SimpleSwiftBLE
import Testing

extension SimpleSwiftBLETests {
    @Suite struct BackendTests {
        @Test func backendsShareAdapterObjects() throws {
            let backend = try #require(try Backend.getBackends().first)
            #expect(try Backend.getBackends().first === backend)
            #expect(try backend.getAdapters().first === Plain.adapter())
        }
    }
}
