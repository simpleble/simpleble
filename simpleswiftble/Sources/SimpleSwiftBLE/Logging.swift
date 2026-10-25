internal import CSimpleBLE

/// The callback that receives SimpleBLE's log entries.
private let logCallback = Locked<(@Sendable (Logging.Entry) -> Void)?>(nil)

/// SimpleBLE's log, for the whole process. By default it goes to standard output, up to the `info` level.
public enum Logging {
    public enum Level: Int, Sendable, Comparable {
        case none = 0
        case fatal
        case error
        case warn
        case info
        case debug
        case verbose

        public static func < (lhs: Level, rhs: Level) -> Bool {
            lhs.rawValue < rhs.rawValue
        }
    }

    public struct Entry: Sendable {
        public let level: Level
        public let module: String
        public let file: String
        public let line: Int
        public let function: String
        public let message: String
    }

    /// The most detailed level that is logged. `none` turns logging off.
    public static var level: Level {
        get { Level(rawValue: Int(simpleble_logging_get_level().rawValue)) ?? .none }
        set { simpleble_logging_set_level(simpleble_log_level_t(rawValue: UInt32(newValue.rawValue))) }
    }

    /// Delivers each entry to `callback`, on the thread that logged it. Pass nil to discard entries.
    public static func setCallback(_ callback: (@Sendable (Entry) -> Void)?) {
        logCallback.withLock { $0 = callback }
        simpleble_logging_set_callback(
            callback == nil
                ? nil
                : { level, module, file, line, function, message in
                    guard let callback = logCallback.withLock({ $0 }) else { return }
                    callback(
                        Entry(
                            level: Level(rawValue: Int(level.rawValue)) ?? .none,
                            module: module.map { String(cString: $0) } ?? "",
                            file: file.map { String(cString: $0) } ?? "",
                            line: Int(line),
                            function: function.map { String(cString: $0) } ?? "",
                            message: message.map { String(cString: $0) } ?? ""))
                })
    }

    /// Appends entries to a file.
    public static func logToFile(_ path: String) {
        logCallback.withLock { $0 = nil }
        simpleble_logging_log_default_file_path(path)
    }

    /// Writes entries to standard output, the default.
    public static func logToStdout() {
        logCallback.withLock { $0 = nil }
        simpleble_logging_log_default_stdout()
    }
}
