using System.Runtime.InteropServices;
using SimpleSharpBLE.Internal;

namespace SimpleSharpBLE.Internal.Interop;

internal static class NativeCallbacks
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Signal(nint handle, nint token);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Found(nint adapter, nint peripheral, nint token);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Data(nint handle, NativeUuid service, NativeUuid characteristic, nint data, nuint length, nint token);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Value(nint handle, nint data, nuint length, nint token);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Text(nint handle, nint text, nint token);

    internal static readonly Signal OnSignal = (_, token) => Guard(() => CallbackSlot.Post(token));
    internal static readonly Found OnFound = (_, pointer, token) => Guard(() =>
    {
        var owned = new NativeHandle(pointer, HandleKind.Peripheral);
        Peripheral peripheral;
        try { peripheral = new Peripheral(owned); }
        catch { owned.Dispose(); throw; }
        CallbackSlot.Post(token, peripheral, peripheral.Dispose);
    });
    internal static readonly Data OnData = (_, _, _, data, length, token) => Guard(() => CallbackSlot.Post(token, Buffers.Copy(data, length)));
    internal static readonly Value OnValue = (_, data, length, token) => Guard(() => CallbackSlot.Post(token, Buffers.Copy(data, length)));
    internal static readonly Text OnText = (_, text, token) => Guard(() => CallbackSlot.Post(token, Marshal.PtrToStringUTF8(text) ?? ""));
    internal static void Guard(Action action)
    {
        try { action(); } catch (Exception ex) { CallbackErrors.Report(ex); }
    }
}
