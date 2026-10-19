using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_peripheral_services(nint h, out nint list, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_service_list_get(nint list, nuint index, out NativeService value, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_service_list_release(nint list);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_service_release(ref NativeService value);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_peripheral_manufacturer_data(nint h, out nint list, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_manufacturer_data_list_get(nint list, nuint index, out NativeManufacturerData value, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_manufacturer_data_list_release(nint list);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_manufacturer_data_release(ref NativeManufacturerData value);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_peripheral_read(nint h, NativeUuid service, NativeUuid characteristic, out nuint length, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nint simpleble_peripheral_read_descriptor(nint h, NativeUuid service, NativeUuid characteristic, NativeUuid descriptor, out nuint length, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_write_request(nint h, NativeUuid service, NativeUuid characteristic, byte[] data, nuint length, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_write_command(nint h, NativeUuid service, NativeUuid characteristic, byte[] data, nuint length, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_write_descriptor(nint h, NativeUuid service, NativeUuid characteristic, NativeUuid descriptor, byte[] data, nuint length, ref nint error);
}

