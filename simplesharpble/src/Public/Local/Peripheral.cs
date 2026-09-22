using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Local;

public sealed class ClientEventArgs(string address) : EventArgs
{
    public string Address { get; } = address;
}

/// <summary>A local GATT host. Configure before Start; explicitly Stop before disposing.</summary>
public sealed class Peripheral : IDisposable
{
    internal readonly NativeHandle handle;
    private readonly CallbackOwner callbacks;
    private readonly object eventGate = new();
    private readonly LocalChildren<LocalChildren<LocalReadState>> children = new();
    private EventHandler<ClientEventArgs>? connected, disconnected;
    internal Peripheral(nint pointer)
    {
        handle = new(pointer, HandleKind.LocalPeripheral);
        callbacks = new(handle);
    }
    public void AddAdvertisedService(string uuid)
    {
        var value = new NativeUuid(uuid);
        handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_advertised_service(h, value, ref e));
    }
    public void AddAdvertisedService(IEnumerable<string> uuids)
    {
        if (uuids is null) throw new ArgumentNullException(nameof(uuids));
        // Validate the entire input before changing the host.
        var values = uuids.Select(x => new NativeUuid(x)).ToArray();
        foreach (var value in values)
            handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_advertised_service(h, value, ref e));
    }
    public Service AddService(string uuid)
    {
        var value = new NativeUuid(uuid);
        lock (children)
        {
            var state = children.At(handle.Query(NativeMethods.simpleble_local_peripheral_services_count));
            return new(handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_service(h, value, ref e)), state);
        }
    }
    public IReadOnlyList<Service> Services
    {
        get
        {
            lock (children)
                return NativeCall.Collect(handle.Query(NativeMethods.simpleble_local_peripheral_services_count),
                    index => new Service(handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_services_get(h, index, ref e)), children.At(index)));
        }
    }
    public bool IsStarted => handle.Query(NativeMethods.simpleble_local_peripheral_is_started);
    public bool IsAdvertising => handle.Query(NativeMethods.simpleble_local_peripheral_is_advertising);
    public void RemoveAllServices()
    {
        lock (children)
        {
            handle.Execute(NativeMethods.simpleble_local_peripheral_remove_all_services);
            children.Clear();
        }
    }
    public void Start() => handle.Execute(NativeMethods.simpleble_local_peripheral_start);
    public void Stop() => handle.Execute(NativeMethods.simpleble_local_peripheral_stop);
    public event EventHandler<ClientEventArgs> ClientConnected
    {
        add { lock (eventGate) SetEvent(true, connected + value); }
        remove { lock (eventGate) SetEvent(true, connected - value); }
    }
    public event EventHandler<ClientEventArgs> ClientDisconnected
    {
        add { lock (eventGate) SetEvent(false, disconnected + value); }
        remove { lock (eventGate) SetEvent(false, disconnected - value); }
    }
    private void SetEvent(bool connect, EventHandler<ClientEventArgs>? handlers)
    {
        callbacks.Set(connect ? "connected" : "disconnected", handlers is null ? null : data =>
        { Events.Raise(handlers, this, new ClientEventArgs((string)data!)); return null; }, (h, id) =>
        {
            if (connect) NativeMethods.simpleble_local_peripheral_set_callback_on_client_connected(h, id == 0 ? null : NativeCallbacks.OnText, id);
            else NativeMethods.simpleble_local_peripheral_set_callback_on_client_disconnected(h, id == 0 ? null : NativeCallbacks.OnText, id);
        });
        if (connect) connected = handlers; else disconnected = handlers;
    }
    public void Dispose()
    {
        callbacks.Dispose();
        lock (eventGate) connected = disconnected = null;
    }
}
