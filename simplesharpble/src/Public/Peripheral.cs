using System.Collections.ObjectModel;
using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>A remote BLE peripheral. Dispose releases this wrapper without implicitly disconnecting.</summary>
public sealed class Peripheral : IDisposable
{
    internal readonly NativeHandle handle;
    internal readonly CallbackOwner callbacks;
    internal Peripheral(nint value) : this(new NativeHandle(value, HandleKind.Peripheral)) { }
    internal Peripheral(NativeHandle handle)
    {
        this.handle = handle;
        callbacks = new(handle);
    }

    public string Identifier => NativeCall.String(handle.Query(NativeMethods.simpleble_peripheral_identifier));
    public string Address => NativeCall.String(handle.Query(NativeMethods.simpleble_peripheral_address));
    public BluetoothAddressType AddressType => handle.Query(NativeMethods.simpleble_peripheral_address_type);
    public short Rssi => handle.Query(NativeMethods.simpleble_peripheral_rssi);
    /// <summary>Advertised transmit power in dBm, or short.MinValue when absent, as in C++.</summary>
    public short TxPower => handle.Query(NativeMethods.simpleble_peripheral_tx_power);
    public ushort Mtu => handle.Query(NativeMethods.simpleble_peripheral_mtu);
    public bool IsConnected => handle.Query(NativeMethods.simpleble_peripheral_is_connected);
    public bool IsConnectable => handle.Query(NativeMethods.simpleble_peripheral_is_connectable);
    public bool IsPaired => handle.Query(NativeMethods.simpleble_peripheral_is_paired);

    public void Connect() => handle.Execute(NativeMethods.simpleble_peripheral_connect);
    public void Disconnect() => handle.Execute(NativeMethods.simpleble_peripheral_disconnect);
    /// <summary>Runs Connect on a worker. Cancellation only applies before the operation starts.</summary>
    public Task ConnectAsync(CancellationToken cancellationToken = default) => Task.Run(Connect, cancellationToken);
    /// <summary>Runs Disconnect on a worker. Cancellation only applies before the operation starts.</summary>
    public Task DisconnectAsync(CancellationToken cancellationToken = default) => Task.Run(Disconnect, cancellationToken);
    public void Unpair() => handle.Execute(NativeMethods.simpleble_peripheral_unpair);

    /// <summary>Advertised services while disconnected, discovered GATT services while connected.</summary>
    public IReadOnlyList<Service> Services
    {
        get
        {
            nuint count = handle.Query(NativeMethods.simpleble_peripheral_services_count);
            var result = new List<Service>(checked((int)count));
            for (nuint i = 0; i < count; i++)
            {
                NativeService value = default;
                try
                {
                    handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_peripheral_services_get(h, i, out value, ref e));
                    var characteristics = Buffers.ReadArray<NativeCharacteristic>(value.Characteristics, value.CharacteristicCount)
                        .Select(c => new Characteristic(c.Uuid.ToString(), c.CanRead != 0, c.CanWriteRequest != 0,
                            c.CanWriteCommand != 0, c.CanNotify != 0, c.CanIndicate != 0,
                            Array.AsReadOnly(Buffers.ReadArray<NativeDescriptor>(c.Descriptors, c.DescriptorCount)
                                .Select(d => new Descriptor(d.Uuid.ToString())).ToArray()))).ToArray();
                    result.Add(new Service(value.Uuid.ToString(), Buffers.Copy(value.Data, value.DataLength), Array.AsReadOnly(characteristics)));
                }
                finally { NativeMethods.simpleble_service_release(ref value); }
            }
            return result.AsReadOnly();
        }
    }

    public IReadOnlyDictionary<ushort, byte[]> ManufacturerData
    {
        get
        {
            nuint count = handle.Query(NativeMethods.simpleble_peripheral_manufacturer_data_count);
            var result = new Dictionary<ushort, byte[]>();
            for (nuint i = 0; i < count; i++)
            {
                NativeManufacturerData value = default;
                try
                {
                    handle.Execute((nint h, ref nint e) => NativeMethods.simpleble_peripheral_manufacturer_data_get(h, i, out value, ref e));
                    result.Add(value.ManufacturerId, Buffers.Copy(value.Data, value.DataLength));
                }
                finally { NativeMethods.simpleble_manufacturer_data_release(ref value); }
            }
            return new ReadOnlyDictionary<ushort, byte[]>(result);
        }
    }

    public byte[] Read(string service, string characteristic) => ReadCore(service, characteristic, null);
    public byte[] Read(string service, string characteristic, string descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        return ReadCore(service, characteristic, descriptor);
    }
    private byte[] ReadCore(string service, string characteristic, string? descriptor)
    {
        var s = new NativeUuid(service); var c = new NativeUuid(characteristic);
        NativeUuid d = descriptor is null ? default : new(descriptor);
        nuint length = 0; nint data = 0;
        try
        {
            data = handle.Query((nint h, ref nint e) => descriptor is null
                ? NativeMethods.simpleble_peripheral_read(h, s, c, out length, ref e)
                : NativeMethods.simpleble_peripheral_read_descriptor(h, s, c, d, out length, ref e));
            return Buffers.Copy(data, length);
        }
        finally { NativeMethods.simpleble_free(data); }
    }

    public void WriteRequest(string service, string characteristic, byte[] data) => WriteCore(service, characteristic, null, data, true);
    public void WriteCommand(string service, string characteristic, byte[] data) => WriteCore(service, characteristic, null, data, false);
    public void Write(string service, string characteristic, string descriptor, byte[] data)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        WriteCore(service, characteristic, descriptor, data, true);
    }
    private void WriteCore(string service, string characteristic, string? descriptor, byte[] data, bool request)
    {
        byte[] input = Buffers.Input(data);
        var s = new NativeUuid(service); var c = new NativeUuid(characteristic);
        NativeUuid d = descriptor is null ? default : new(descriptor);
        handle.Execute((nint h, ref nint e) =>
        {
            if (descriptor is not null) NativeMethods.simpleble_peripheral_write_descriptor(h, s, c, d, input, (nuint)data.Length, ref e);
            else if (request) NativeMethods.simpleble_peripheral_write_request(h, s, c, input, (nuint)data.Length, ref e);
            else NativeMethods.simpleble_peripheral_write_command(h, s, c, input, (nuint)data.Length, ref e);
        });
    }

    private readonly object eventGate = new();
    private EventHandler? connected, disconnected;
    public event EventHandler Connected
    {
        add { lock (eventGate) { SetConnectionEvent(true, connected + value); } }
        remove { lock (eventGate) { SetConnectionEvent(true, connected - value); } }
    }
    public event EventHandler Disconnected
    {
        add { lock (eventGate) { SetConnectionEvent(false, disconnected + value); } }
        remove { lock (eventGate) { SetConnectionEvent(false, disconnected - value); } }
    }
    private void SetConnectionEvent(bool connect, EventHandler? handlers)
    {
        callbacks.Set(connect ? "connected" : "disconnected", handlers is null ? null : _ =>
        { Events.Raise(handlers, this); return null; }, (h, id) =>
        {
            if (connect) NativeMethods.simpleble_peripheral_set_callback_on_connected(h, id == 0 ? null : NativeCallbacks.OnSignal, id);
            else NativeMethods.simpleble_peripheral_set_callback_on_disconnected(h, id == 0 ? null : NativeCallbacks.OnSignal, id);
        });
        if (connect) connected = handlers; else disconnected = handlers;
    }

    public void Notify(string service, string characteristic, Action<byte[]> callback) => Subscribe(service, characteristic, callback, false);
    public void Indicate(string service, string characteristic, Action<byte[]> callback) => Subscribe(service, characteristic, callback, true);
    private void Subscribe(string service, string characteristic, Action<byte[]> callback, bool indicate)
    {
        ArgumentNullException.ThrowIfNull(callback);
        var s = new NativeUuid(service); var c = new NativeUuid(characteristic);
        callbacks.Set($"data:{s}:{c}", data => { callback((byte[])data!); return null; }, (h, id) =>
            NativeCall.Invoke((ref nint error) =>
            {
                if (indicate) NativeMethods.simpleble_peripheral_indicate(h, s, c, NativeCallbacks.OnData, id, ref error);
                else NativeMethods.simpleble_peripheral_notify(h, s, c, NativeCallbacks.OnData, id, ref error);
                return 0;
            }));
    }

    /// <summary>Stops the native subscription, including one installed through another wrapper of the same device.</summary>
    public void Unsubscribe(string service, string characteristic)
    {
        var s = new NativeUuid(service); var c = new NativeUuid(characteristic);
        callbacks.Set($"data:{s}:{c}", null, (h, _) => NativeCall.Invoke((ref nint error) =>
        {
            NativeMethods.simpleble_peripheral_unsubscribe(h, s, c, ref error);
            return 0;
        }));
    }

    public void Dispose()
    {
        callbacks.Dispose();
        lock (eventGate) connected = disconnected = null;
    }
}
