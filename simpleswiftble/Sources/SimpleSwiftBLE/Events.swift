/// Delivers each event to every stream made from it, in the order the events happened.
final class EventSource<Element: Sendable>: Sendable {
    private struct State {
        var nextID: UInt64 = 0
        var continuations: [UInt64: AsyncStream<Element>.Continuation] = [:]
    }

    private let state = Locked(State())

    /// A stream that buffers every event from now on, until it is dropped or its iteration is cancelled.
    func stream() -> AsyncStream<Element> {
        let (stream, continuation) = AsyncStream.makeStream(of: Element.self, bufferingPolicy: .unbounded)
        let id = state.withLock { state in
            state.nextID += 1
            state.continuations[state.nextID] = continuation
            return state.nextID
        }
        continuation.onTermination = { [weak self] _ in
            self?.state.withLock { state in
                state.continuations[id] = nil
            }
        }
        return stream
    }

    func yield(_ value: Element) {
        let continuations = state.withLock { state in Array(state.continuations.values) }
        for continuation in continuations {
            continuation.yield(value)
        }
    }
}
