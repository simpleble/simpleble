using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>Native log severity, from None (off) to Verbose.</summary>
public enum LogLevel
{
    /// <summary>No logging.</summary>
    None,
    /// <summary>Unrecoverable failures.</summary>
    Fatal,
    /// <summary>Failed operations.</summary>
    Error,
    /// <summary>Unexpected but recoverable conditions.</summary>
    Warn,
    /// <summary>Significant state changes.</summary>
    Info,
    /// <summary>Diagnostic detail.</summary>
    Debug,
    /// <summary>All messages.</summary>
    Verbose,
}

/// <summary>One native log entry.</summary>
/// <param name="Level">The entry's severity.</param>
/// <param name="Module">The native component that logged the entry.</param>
/// <param name="File">The native source file.</param>
/// <param name="Line">The native source line.</param>
/// <param name="Function">The native function.</param>
/// <param name="Message">The log text.</param>
public sealed record LogMessage(LogLevel Level, string Module, string File, uint Line, string Function, string Message);

/// <summary>Process-wide native logging. Managed callbacks run on a worker thread.</summary>
public static class Logging
{
    private static readonly object gate = new();
    private static CallbackSlot? slot;
    private static readonly NativeMethods.LogCallback trampoline = LogCallback;
    [MonoPInvokeCallback(typeof(NativeMethods.LogCallback))]
    private static void LogCallback(LogLevel level, nint module, nint file, uint line, nint function, nint message) =>
        NativeCallbacks.Guard(() =>
        {
            var current = Volatile.Read(ref slot);
            if (current is not null) CallbackSlot.Post(current.Id, new LogMessage(level, Text(module), Text(file), line, Text(function), Text(message)));
        });
    private static string Text(nint value) => Marshal.PtrToStringUTF8(value) ?? "";
    /// <summary>The least severe level that is logged. None turns logging off.</summary>
    public static LogLevel Level
    {
        get => NativeMethods.simpleble_logging_get_level();
        set { if (!Enum.IsDefined(typeof(LogLevel), value)) throw new ArgumentOutOfRangeException(nameof(value)); NativeMethods.simpleble_logging_set_level(value); }
    }
    /// <summary>Whether a log sink is installed.</summary>
    public static bool HasCallback => NativeMethods.simpleble_logging_has_callback();
    /// <summary>Forwards native log entries to callback on a worker thread, or removes the sink when null.</summary>
    public static void SetCallback(Action<LogMessage>? callback)
    {
        lock (gate)
        {
            var replacement = callback is null ? null : new CallbackSlot(value => { callback((LogMessage)value!); return null; });
            NativeMethods.simpleble_logging_set_callback(callback is null ? null : trampoline);
            Interlocked.Exchange(ref slot, replacement)?.Dispose();
        }
    }
    /// <summary>Writes native log entries to standard output.</summary>
    public static void LogDefaultStdout()
    {
        lock (gate) { NativeMethods.simpleble_logging_log_default_stdout(); Interlocked.Exchange(ref slot, null)?.Dispose(); }
    }
    /// <summary>Appends native log entries to path, or to a timestamp-named file in the working directory when null.</summary>
    public static void LogDefaultFile(string? path = null)
    {
        if (path?.Contains('\0') == true) throw new ArgumentException("A path cannot contain NUL.", nameof(path));
        lock (gate) { NativeMethods.simpleble_logging_log_default_file_path(path); Interlocked.Exchange(ref slot, null)?.Dispose(); }
    }
}
