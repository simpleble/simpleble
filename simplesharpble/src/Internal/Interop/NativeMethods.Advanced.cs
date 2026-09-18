using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal delegate bool PairCallback(nint h, nint text, nint token);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_dongl_set_passkey_request_callback(nint h, PairCallback? callback, nint token, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_dongl_set_numeric_comparison_callback(nint h, PairCallback? callback, nint token, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_dongl_set_passkey_display_callback(nint h, NativeCallbacks.Text? callback, nint token, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_macos_set_advertisement_local_name(nint h, [MarshalAs(UnmanagedType.LPUTF8Str)] string? name, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_ios_set_advertisement_local_name(nint h, [MarshalAs(UnmanagedType.LPUTF8Str)] string? name, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_advanced_linux_set_advertisement_local_name(nint h, [MarshalAs(UnmanagedType.LPUTF8Str)] string? name, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_advanced_macos_retrieve_cached_peripheral(nint h, [MarshalAs(UnmanagedType.LPUTF8Str)] string identifier, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_advanced_ios_retrieve_cached_peripheral(nint h, [MarshalAs(UnmanagedType.LPUTF8Str)] string identifier, ref nint error);
}

