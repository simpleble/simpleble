using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>The platform the native library was built for.</summary>
public enum NativeOperatingSystem
{
    /// <summary>Windows.</summary>
    Windows,
    /// <summary>macOS.</summary>
    MacOS,
    /// <summary>Linux.</summary>
    Linux,
    /// <summary>iOS, iPadOS, or Mac Catalyst.</summary>
    IOS,
    /// <summary>Android.</summary>
    Android,
    /// <summary>Any other platform.</summary>
    Unknown,
}

/// <summary>Native library information.</summary>
public static class Utils
{
    /// <summary>The native SimpleBLE version.</summary>
    public static string Version => Marshal.PtrToStringUTF8(NativeMethods.simpleble_get_version()) ?? "";
    /// <summary>The platform the native library was built for.</summary>
    public static NativeOperatingSystem OperatingSystem => NativeMethods.simpleble_get_operating_system();
}

