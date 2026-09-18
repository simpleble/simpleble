using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>A SimpleBLE backend. Dispose releases this wrapper, not its adapters.</summary>
public sealed class Backend : IDisposable
{
    private readonly NativeHandle handle;
    internal Backend(nint value) => handle = new(value, HandleKind.Backend);

    public string Identifier => NativeCall.String(handle.Query(NativeMethods.simpleble_backend_identifier));
    public bool IsBluetoothEnabled => handle.Query(NativeMethods.simpleble_backend_is_bluetooth_enabled);

    /// <summary>Initializes available backends. Dispose each returned backend.</summary>
    public static IReadOnlyList<Backend> GetBackends() => NativeCall.Collect(
        NativeCall.Invoke(NativeMethods.simpleble_backend_get_count),
        index => new Backend(NativeCall.Invoke((ref nint error) =>
            NativeMethods.simpleble_backend_get_handle(index, ref error))));

    /// <summary>Returns independently owned wrappers. Dispose each returned adapter.</summary>
    public IReadOnlyList<Adapter> Adapters => NativeCall.Collect(
        handle.Query(NativeMethods.simpleble_backend_get_adapters_count),
        index => new Adapter(handle.Query((nint value, ref nint error) =>
            NativeMethods.simpleble_backend_get_adapters_handle(value, index, ref error))));

    public void Dispose() => handle.Dispose();
}
