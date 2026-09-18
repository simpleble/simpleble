using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Local;

[Flags]
public enum CharacteristicCapability { Read = 1, WriteRequest = 2, WriteCommand = 4, Notify = 8, Indicate = 16 }

public sealed class ValueEventArgs(byte[] value) : EventArgs
{
    public byte[] Value { get; } = value;
}

public sealed class Characteristic : IDisposable
{
    internal readonly NativeHandle handle;
    private readonly CallbackOwner callbacks;
    private readonly object eventGate = new();
    private EventHandler<ValueEventArgs>? written;
    private EventHandler? subscribed, unsubscribed;
    private readonly LocalReadState readState;
    private LocalReadRegistration? read;
    internal Characteristic(nint pointer, LocalReadState readState)
    {
        this.readState = readState;
        handle = new(pointer, HandleKind.LocalCharacteristic);
        callbacks = new(handle);
    }
    public string Uuid
    {
        get
        {
            NativeUuid uuid = default;
            handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_characteristic_uuid(h, out uuid, ref e));
            return uuid.ToString();
        }
    }
    public CharacteristicCapability Capabilities => (CharacteristicCapability)handle.Query(NativeMethods.simpleble_local_characteristic_capabilities);
    public byte[] Value
    {
        get
        {
            nint data = 0; nuint length = 0;
            try
            {
                data = handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_characteristic_value(h, out length, ref e));
                return Buffers.Copy(data, length);
            }
            finally { NativeMethods.simpleble_free(data); }
        }
        set
        {
            byte[] input = Buffers.Input(value);
            handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_characteristic_set_value(h, input, (nuint)value.Length, ref e));
        }
    }

    /// <summary>Synchronous dynamic read handler. Null uses the stored value. Do not change callbacks from the handler.</summary>
    public Func<byte[]>? ReadHandler
    {
        get => Volatile.Read(ref read)?.Handler;
        set
        {
            LocalReadRegistration.CheckMutation();
            lock (eventGate)
            {
                var next = LocalReadRegistration.Set(handle, readState, value);
                read?.Dispose();
                read = next;
            }
        }
    }
    public event EventHandler<ValueEventArgs> Written
    {
        add { LocalReadRegistration.CheckMutation(); lock (eventGate) SetWritten(written + value); }
        remove { LocalReadRegistration.CheckMutation(); lock (eventGate) SetWritten(written - value); }
    }
    private void SetWritten(EventHandler<ValueEventArgs>? handlers)
    {
        callbacks.Set("written", handlers is null ? null : value =>
        { Events.Raise(handlers, this, new ValueEventArgs((byte[])value!)); return null; },
            (h, id) => NativeMethods.simpleble_local_characteristic_set_callback_on_write(h, id == 0 ? null : NativeCallbacks.OnValue, id));
        written = handlers;
    }
    public event EventHandler Subscribed
    {
        add { LocalReadRegistration.CheckMutation(); lock (eventGate) SetSubscription(true, subscribed + value); }
        remove { LocalReadRegistration.CheckMutation(); lock (eventGate) SetSubscription(true, subscribed - value); }
    }
    public event EventHandler Unsubscribed
    {
        add { LocalReadRegistration.CheckMutation(); lock (eventGate) SetSubscription(false, unsubscribed + value); }
        remove { LocalReadRegistration.CheckMutation(); lock (eventGate) SetSubscription(false, unsubscribed - value); }
    }
    private void SetSubscription(bool subscribe, EventHandler? handlers)
    {
        callbacks.Set(subscribe ? "subscribed" : "unsubscribed", handlers is null ? null : _ =>
        { Events.Raise(handlers, this); return null; }, (h, id) =>
        {
            if (subscribe) NativeMethods.simpleble_local_characteristic_set_callback_on_subscribed(h, id == 0 ? null : NativeCallbacks.OnSignal, id);
            else NativeMethods.simpleble_local_characteristic_set_callback_on_unsubscribed(h, id == 0 ? null : NativeCallbacks.OnSignal, id);
        });
        if (subscribe) subscribed = handlers; else unsubscribed = handlers;
    }
    public void Dispose()
    {
        LocalReadRegistration.CheckMutation();
        lock (eventGate)
        {
            read?.Dispose();
            read = null;
            callbacks.Dispose();
            written = null; subscribed = unsubscribed = null;
        }
    }
}
