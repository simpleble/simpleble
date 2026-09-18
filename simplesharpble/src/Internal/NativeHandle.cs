using Microsoft.Win32.SafeHandles;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Internal;

internal enum HandleKind { Backend, Adapter, Peripheral, LocalPeripheral, LocalService, LocalCharacteristic }

internal sealed class NativeHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    private readonly HandleKind kind;
    private int disposed;

    internal NativeHandle(nint value, HandleKind kind) : base(ownsHandle: true)
    {
        if (value == 0 || value == -1)
            throw new InvalidOperationException("Native code returned an invalid handle without an error.");
        this.kind = kind;
        SetHandle(value);
    }

    internal T Query<T>(HandleQuery<T> query)
    {
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
        bool acquired = false;
        try
        {
            DangerousAddRef(ref acquired);
            return NativeCall.Invoke((ref nint error) => query(DangerousGetHandle(), ref error));
        }
        finally
        {
            if (acquired) DangerousRelease();
        }
    }

    protected override void Dispose(bool disposing)
    {
        Interlocked.Exchange(ref disposed, 1);
        base.Dispose(disposing);
    }

    internal void Execute(HandleCommand command) => Query((nint value, ref nint error) =>
    {
        command(value, ref error);
        return 0;
    });

    protected override bool ReleaseHandle()
    {
        switch (kind)
        {
            case HandleKind.Backend: NativeMethods.simpleble_backend_release_handle(handle); break;
            case HandleKind.Adapter: NativeMethods.simpleble_adapter_release_handle(handle); break;
            case HandleKind.Peripheral: NativeMethods.simpleble_peripheral_release_handle(handle); break;
            case HandleKind.LocalPeripheral: NativeMethods.simpleble_local_peripheral_release_handle(handle); break;
            case HandleKind.LocalService: NativeMethods.simpleble_local_service_release_handle(handle); break;
            case HandleKind.LocalCharacteristic: NativeMethods.simpleble_local_characteristic_release_handle(handle); break;
        }
        return true;
    }
}
