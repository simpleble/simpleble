internal import CSimpleBLE

/// Every backend seen so far, by identifier, since SimpleBLE has one backend per identifier.
private let backends = Locked<[String: Backend]>([:])

/// A source of adapters: the operating system's Bluetooth stack, Dongls, or the simulator.
public final class Backend: Sendable {
    let handle: NativeHandle
    public let identifier: String

    private init(handle: NativeHandle, identifier: String) {
        self.handle = handle
        self.identifier = identifier
    }

    /// Every active backend.
    public static func getBackends() throws -> [Backend] {
        var list: simpleble_backend_list_t?
        let count = try check { simpleble_backend_get_backends(&list, $0) }
        defer { simpleble_backend_list_release(list) }
        return try (0..<count).map { index in
            try intern(try check { simpleble_backend_list_get(list, index, $0) }!)
        }
    }

    /// The operating system's Bluetooth backend.
    public static func getNative() throws -> Backend {
        try intern(try check { simpleble_backend_get_native($0) }!)
    }

    /// The Dongl backend.
    public static func getDongl() throws -> Backend {
        try intern(try check { simpleble_backend_get_dongl($0) }!)
    }

    /// The simulator backend.
    public static func getSimulator() throws -> Backend {
        try intern(try check { simpleble_backend_get_simulator($0) }!)
    }

    public func isBluetoothEnabled() throws -> Bool {
        try check { simpleble_backend_is_bluetooth_enabled(handle.raw, $0) }
    }

    public func getAdapters() throws -> [Adapter] {
        var list: simpleble_adapter_list_t?
        let count = try check { simpleble_backend_get_adapters(handle.raw, &list, $0) }
        defer { simpleble_adapter_list_release(list) }
        return try (0..<count).map { index in
            Adapter.intern(try check { simpleble_adapter_list_get(list, index, $0) }!)
        }
    }

    /// Returns the backend's object, keeping the first handle to it and releasing later ones.
    private static func intern(_ raw: UnsafeMutableRawPointer) throws -> Backend {
        let identifier: String
        do {
            identifier = takeString(try check { simpleble_backend_identifier(raw, $0) })
        } catch {
            simpleble_backend_release_handle(raw)
            throw error
        }
        let (backend, created) = backends.withLock { backends in
            if let backend = backends[identifier] {
                return (backend, false)
            }
            let backend = Backend(handle: NativeHandle(raw: raw), identifier: identifier)
            backends[identifier] = backend
            return (backend, true)
        }
        if !created {
            simpleble_backend_release_handle(raw)
        }
        return backend
    }
}

extension Backend: Hashable, Identifiable {
    public static func == (lhs: Backend, rhs: Backend) -> Bool {
        lhs === rhs
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(ObjectIdentifier(self))
    }
}
