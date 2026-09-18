using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Internal;

internal delegate T NativeQuery<T>(ref nint error);
internal delegate T HandleQuery<T>(nint handle, ref nint error);
internal delegate void HandleCommand(nint handle, ref nint error);

internal static class NativeCall
{
    internal static T Invoke<T>(NativeQuery<T> query)
    {
        nint error = 0;
        try
        {
            T result = query(ref error);
            if (error != 0)
            {
                throw new BleException(NativeMethods.simpleble_error_code(error),
                    Marshal.PtrToStringUTF8(NativeMethods.simpleble_error_message(error))
                    ?? "Native BLE operation failed.");
            }
            return result;
        }
        finally
        {
            NativeMethods.simpleble_error_release(ref error);
        }
    }

    internal static string String(nint value)
    {
        try
        {
            return Marshal.PtrToStringUTF8(value)
                ?? throw new InvalidOperationException("Native code returned a null string without an error.");
        }
        finally
        {
            NativeMethods.simpleble_free(value);
        }
    }

    // No partial collection escapes if any native retrieval fails.
    internal static IReadOnlyList<T> Collect<T>(nuint count, Func<nuint, T> create) where T : IDisposable
    {
        var items = new List<T>(checked((int)count));
        try
        {
            for (nuint i = 0; i < count; i++) items.Add(create(i));
            return items.AsReadOnly();
        }
        catch
        {
            foreach (T item in items) item.Dispose();
            throw;
        }
    }
}
