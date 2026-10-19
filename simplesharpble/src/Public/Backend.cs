using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>A SimpleBLE backend. Dispose releases this wrapper, not its adapters.</summary>
public sealed class Backend : IDisposable
{
    private readonly NativeHandle handle;
    internal Backend(nint value) => handle = new(value, HandleKind.Backend);

    /// <summary>The backend name, such as WinRT, CoreBluetooth, SimpleBluez, Android, or Dongl.</summary>
    public string Identifier => NativeCall.String(handle.Query(NativeMethods.simpleble_backend_identifier));
    /// <summary>Whether this backend reports Bluetooth as enabled.</summary>
    public bool IsBluetoothEnabled => handle.Query(NativeMethods.simpleble_backend_is_bluetooth_enabled);

    /// <summary>Initializes available backends. Dispose each returned backend.</summary>
    public static IReadOnlyList<Backend> GetBackends()
    {
        nint list = 0;
        nuint count = NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_backend_get_backends(out list, ref error));
        return NativeCall.Collect(list, count, NativeMethods.simpleble_backend_list_release, (value, index) =>
            new Backend(NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_backend_list_get(value, index, ref error))));
    }

    /// <summary>The backend of the operating system's Bluetooth stack. Dispose the returned backend.</summary>
    public static Backend GetNative() => new(NativeCall.Invoke(NativeMethods.simpleble_backend_get_native));

    /// <summary>
    /// The Dongl backend, whether or not Config.Dongl.UseDonglBackend lists it in GetBackends(). Throws
    /// BleException with OperationNotSupported if this build of SimpleBLE doesn't include it. Dispose the returned
    /// backend.
    /// </summary>
    public static Backend GetDongl() => new(NativeCall.Invoke(NativeMethods.simpleble_backend_get_dongl));

    /// <summary>
    /// The Simulator backend, which only has adapters while a simulation is active. Throws BleException with
    /// OperationNotSupported if this build of SimpleBLE doesn't include it. Dispose the returned backend.
    /// </summary>
    public static Backend GetSimulator() => new(NativeCall.Invoke(NativeMethods.simpleble_backend_get_simulator));

    /// <summary>Returns independently owned wrappers. Dispose each returned adapter.</summary>
    public IReadOnlyList<Adapter> Adapters
    {
        get
        {
            nint list = 0;
            nuint count = handle.Query((nint value, ref nint error) => NativeMethods.simpleble_backend_get_adapters(value, out list, ref error));
            return NativeCall.Collect(list, count, NativeMethods.simpleble_adapter_list_release, (value, index) =>
                new Adapter(NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_adapter_list_get(value, index, ref error))));
        }
    }

    /// <summary>Releases this wrapper.</summary>
    public void Dispose() => handle.Dispose();
}
