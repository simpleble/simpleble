using System.Runtime.InteropServices;

namespace SimpleSharpBLE.Internal;

internal static class Buffers
{
    internal static byte[] Copy(nint data, nuint length)
    {
        int count = checked((int)length);
        if (count == 0) return [];
        if (data == 0) throw new InvalidOperationException("Native data pointer is null with a nonzero length.");
        byte[] result = new byte[count];
        Marshal.Copy(data, result, 0, count);
        return result;
    }

    internal static unsafe T[] ReadArray<T>(nint data, nuint length) where T : unmanaged
    {
        int count = checked((int)length);
        if (count == 0) return [];
        if (data == 0) throw new InvalidOperationException("Native array pointer is null with a nonzero length.");
        return new ReadOnlySpan<T>((void*)data, count).ToArray();
    }

    internal static byte[] Input(byte[] data)
    {
        if (data is null) throw new ArgumentNullException(nameof(data));
        // Several existing C writes require non-NULL even for a zero-length write.
        return data.Length == 0 ? new byte[1] : data;
    }
}
