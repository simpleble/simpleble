using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

public enum NativeOperatingSystem { Windows, MacOS, Linux, IOS, Android, Unknown }

public static class Utils
{
    public static string Version => Marshal.PtrToStringUTF8(NativeMethods.simpleble_get_version()) ?? "";
    public static NativeOperatingSystem OperatingSystem => NativeMethods.simpleble_get_operating_system();
}

