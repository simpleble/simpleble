using System.Runtime.InteropServices;
using Xunit;

[assembly: CollectionBehavior(DisableTestParallelization = true)]

namespace SimpleSharpBLE.Tests;

internal static class NativeFixture
{
    private const string Library = "simplesharpble_test_native";
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern nint test_remote();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern nint test_clone_remote(nint handle);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern void test_emit(nint handle);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern nint test_local();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern nuint test_read_local(nint handle, byte[] target, nuint capacity);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern void test_write_local(nint handle, byte[] data, nuint length);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern nuint test_layout(int index);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)] internal static extern void test_log();
}
