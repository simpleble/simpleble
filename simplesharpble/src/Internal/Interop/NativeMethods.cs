using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    private const string SharedLibrary = "simplecble";
#if SIMPLESHARPBLE_STATIC
    private const string Library = "__Internal";
#else
    private const string Library = SharedLibrary;
#endif

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_free(nint value);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern BleErrorCode simpleble_error_code(nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_error_message(nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_error_release(ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_backend_get_count(ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_backend_get_handle(nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_backend_release_handle(nint handle);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_backend_identifier(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_backend_is_bluetooth_enabled(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_backend_get_adapters_count(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_backend_get_adapters_handle(nint handle, nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_adapter_is_bluetooth_enabled(ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_adapter_get_count(ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_get_handle(nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_release_handle(nint handle);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_identifier(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_address(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_power_on(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_power_off(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_scan_start(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_scan_stop(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_adapter_is_powered(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_adapter_scan_is_active(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_adapter_scan_for(nint handle, int timeoutMs, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_adapter_scan_get_results_count(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_scan_get_results_handle(nint handle, nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_adapter_get_paired_peripherals_count(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_get_paired_peripherals_handle(nint handle, nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_adapter_get_connected_peripherals_count(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_adapter_get_connected_peripherals_handle(nint handle, nuint index, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_release_handle(nint handle);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_peripheral_identifier(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_peripheral_address(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern BluetoothAddressType simpleble_peripheral_address_type(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern short simpleble_peripheral_rssi(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern short simpleble_peripheral_tx_power(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern ushort simpleble_peripheral_mtu(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_peripheral_is_connected(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_peripheral_is_connectable(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    internal static extern bool simpleble_peripheral_is_paired(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_connect(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_disconnect(nint handle, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_unpair(nint handle, ref nint error);
}
