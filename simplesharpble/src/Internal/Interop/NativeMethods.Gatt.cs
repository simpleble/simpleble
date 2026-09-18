using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal.Interop;

internal static partial class NativeMethods
{
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_peripheral_services_count(nint h, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_services_get(nint h, nuint index, out NativeService value, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_service_release(ref NativeService value);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern nuint simpleble_peripheral_manufacturer_data_count(nint h, ref nint error);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    internal static extern void simpleble_peripheral_manufacturer_data_get(nint h, nuint index, out NativeManufacturerData value, ref nint error);

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

