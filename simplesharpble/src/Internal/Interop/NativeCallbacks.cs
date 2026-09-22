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

    internal static readonly Signal OnSignal = SignalCallback;
    internal static readonly Found OnFound = FoundCallback;
    internal static readonly Data OnData = DataCallback;
    internal static readonly Value OnValue = ValueCallback;
    internal static readonly Text OnText = TextCallback;

    [MonoPInvokeCallback(typeof(Signal))]
    private static void SignalCallback(nint handle, nint token) => Guard(() => CallbackSlot.Post(token));
    [MonoPInvokeCallback(typeof(Found))]
    private static void FoundCallback(nint adapter, nint pointer, nint token) => Guard(() =>
    {
        var owned = new NativeHandle(pointer, HandleKind.Peripheral);
        Peripheral peripheral;
        try { peripheral = new Peripheral(owned); }
        catch { owned.Dispose(); throw; }
        CallbackSlot.Post(token, peripheral, peripheral.Dispose);
    });
    [MonoPInvokeCallback(typeof(Data))]
    private static void DataCallback(nint handle, NativeUuid service, NativeUuid characteristic, nint data, nuint length, nint token) => Guard(() => CallbackSlot.Post(token, Buffers.Copy(data, length)));
    [MonoPInvokeCallback(typeof(Value))]
    private static void ValueCallback(nint handle, nint data, nuint length, nint token) => Guard(() => CallbackSlot.Post(token, Buffers.Copy(data, length)));
    [MonoPInvokeCallback(typeof(Text))]
    private static void TextCallback(nint handle, nint text, nint token) => Guard(() => CallbackSlot.Post(token, Marshal.PtrToStringUTF8(text) ?? ""));
    internal static void Guard(Action action)
    {
        try { action(); } catch (Exception ex) { CallbackErrors.Report(ex); }
    }
}
