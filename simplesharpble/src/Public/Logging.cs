using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

public enum LogLevel { None, Fatal, Error, Warn, Info, Debug, Verbose }
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
    public static LogLevel Level
    {
        get => NativeMethods.simpleble_logging_get_level();
        set { if (!Enum.IsDefined(typeof(LogLevel), value)) throw new ArgumentOutOfRangeException(nameof(value)); NativeMethods.simpleble_logging_set_level(value); }
    }
    public static bool HasCallback => NativeMethods.simpleble_logging_has_callback();
    public static void SetCallback(Action<LogMessage>? callback)
    {
        lock (gate)
        {
            var replacement = callback is null ? null : new CallbackSlot(value => { callback((LogMessage)value!); return null; });
            NativeMethods.simpleble_logging_set_callback(callback is null ? null : trampoline);
            Interlocked.Exchange(ref slot, replacement)?.Dispose();
        }
    }
    public static void LogDefaultStdout()
    {
        lock (gate) { NativeMethods.simpleble_logging_log_default_stdout(); Interlocked.Exchange(ref slot, null)?.Dispose(); }
    }
    public static void LogDefaultFile(string? path = null)
    {
        if (path?.Contains('\0') == true) throw new ArgumentException("A path cannot contain NUL.", nameof(path));
        lock (gate) { NativeMethods.simpleble_logging_log_default_file_path(path); Interlocked.Exchange(ref slot, null)?.Dispose(); }
    }
}
