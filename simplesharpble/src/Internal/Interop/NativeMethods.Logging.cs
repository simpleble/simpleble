using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void LogCallback(LogLevel level, nint module, nint file, uint line, nint function, nint message);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void simpleble_logging_set_level(LogLevel level);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    internal static extern LogLevel simpleble_logging_get_level();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void simpleble_logging_set_callback(LogCallback? callback);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_logging_has_callback();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void simpleble_logging_log_default_stdout();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void simpleble_logging_log_default_file_path([MarshalAs(UnmanagedType.LPUTF8Str)] string? path);
}
