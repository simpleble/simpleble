using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_power_on(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_power_off(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_scan_start(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_scan_stop(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_scan_found(nint h, NativeCallbacks.Found? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_set_callback_on_scan_updated(nint h, NativeCallbacks.Found? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_set_callback_on_connected(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_set_callback_on_disconnected(nint h, NativeCallbacks.Signal? callback, nint token);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_notify(nint h, NativeUuid service, NativeUuid characteristic, NativeCallbacks.Data callback, nint token, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_indicate(nint h, NativeUuid service, NativeUuid characteristic, NativeCallbacks.Data callback, nint token, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_unsubscribe(nint h, NativeUuid service, NativeUuid characteristic, ref nint error);
}

