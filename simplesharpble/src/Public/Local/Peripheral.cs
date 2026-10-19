using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE.Local;

/// <summary>Carries the client of a local GATT host.</summary>
public sealed class ClientEventArgs(string address) : EventArgs
{
    /// <summary>The client's address, or a platform identifier on macOS and iOS.</summary>
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
    /// <summary>Adds a service UUID to the advertisement. Call before Start.</summary>
    public void AddAdvertisedService(string uuid)
    {
        var value = new NativeUuid(uuid);
        handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_advertised_service(h, value, ref e));
    }
    /// <summary>Adds service UUIDs to the advertisement after validating all of them. Call before Start.</summary>
    public void AddAdvertisedService(IEnumerable<string> uuids)
    {
        if (uuids is null) throw new ArgumentNullException(nameof(uuids));
        // Validate the entire input before changing the host.
        var values = uuids.Select(x => new NativeUuid(x)).ToArray();
        foreach (var value in values)
            handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_advertised_service(h, value, ref e));
    }
    /// <summary>Adds manufacturer specific data to the advertisement. Setting data for the same company identifier again replaces it. Call before Start.</summary>
    public void SetAdvertisedManufacturerData(ushort companyId, byte[] data)
    {
        byte[] input = Buffers.Input(data);
        handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_set_advertised_manufacturer_data(h, companyId, input, (nuint)data.Length, ref e));
    }
    /// <summary>Chooses whether centrals can connect. Local peripherals are connectable by default. Call before Start.</summary>
    public void SetConnectable(bool connectable) =>
        handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_set_connectable(h, connectable, ref e));
    /// <summary>Adds a primary service. Call before Start; dispose the returned wrapper.</summary>
    public Service AddService(string uuid)
    {
        var value = new NativeUuid(uuid);
        lock (children)
        {
            nint service = handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_add_service(h, value, ref e));
            return new(service, children.Add(value.ToString()));
        }
    }
    /// <summary>Returns wrappers for the configured services. Dispose each one.</summary>
    public IReadOnlyList<Service> Services
    {
        get
        {
            lock (children)
            {
                var seen = new Dictionary<string, int>();
                nint list = 0;
                nuint count = handle.Query((nint h, ref nint e) => NativeMethods.simpleble_local_peripheral_services(h, out list, ref e));
                return NativeCall.Collect(list, count, NativeMethods.simpleble_local_service_list_release, (value, index) =>
                {
                    nint service = NativeCall.Invoke((ref nint e) => NativeMethods.simpleble_local_service_list_get(value, index, ref e));
                    NativeUuid uuid = default;
                    try { NativeCall.Invoke((ref nint e) => { NativeMethods.simpleble_local_service_uuid(service, out uuid, ref e); return 0; }); }
                    catch { NativeMethods.simpleble_local_service_release_handle(service); throw; }
                    return new Service(service, children.At(uuid.ToString(), seen));
                });
            }
        }
    }
    /// <summary>Whether Start has published the services.</summary>
    public bool IsStarted => handle.Query(NativeMethods.simpleble_local_peripheral_is_started);
    /// <summary>Whether the host is advertising.</summary>
    public bool IsAdvertising => handle.Query(NativeMethods.simpleble_local_peripheral_is_advertising);
    /// <summary>Removes every configured service so the layout can be rebuilt. Call while stopped.</summary>
    public void RemoveAllServices()
    {
        lock (children)
        {
            handle.Execute(NativeMethods.simpleble_local_peripheral_remove_all_services);
            children.Clear();
        }
    }
    /// <summary>Publishes the configured services and starts advertising.</summary>
    public void Start() => handle.Execute(NativeMethods.simpleble_local_peripheral_start);
    /// <summary>Stops advertising and hosting.</summary>
    public void Stop() => handle.Execute(NativeMethods.simpleble_local_peripheral_stop);
    /// <summary>Raised when a client connects. CoreBluetooth reports a client's first subscription instead.</summary>
    public event EventHandler<ClientEventArgs> ClientConnected
    {
        add { lock (eventGate) SetEvent(true, connected + value); }
        remove { lock (eventGate) SetEvent(true, connected - value); }
    }
    /// <summary>Raised when a client disconnects. CoreBluetooth reports a client's last unsubscription instead.</summary>
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
    /// <summary>Releases this wrapper and its event registrations. Stop the host first.</summary>
    public void Dispose()
    {
        callbacks.Dispose();
        lock (eventGate) connected = disconnected = null;
    }
}
