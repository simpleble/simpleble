using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>A Bluetooth adapter. Dispose releases the wrapper; it does not stop scanning or power off.</summary>
public sealed class Adapter : IDisposable
{
    internal readonly NativeHandle handle;
    internal readonly CallbackOwner callbacks;
    internal Adapter(nint value)
    {
        handle = new(value, HandleKind.Adapter);
        callbacks = new(handle);
    }

    /// <summary>The platform's name for the adapter, such as hci0 on Linux.</summary>
    public string Identifier => NativeCall.String(handle.Query(NativeMethods.simpleble_adapter_identifier));
    /// <summary>The adapter's Bluetooth address, as reported by the platform.</summary>
    public string Address => NativeCall.String(handle.Query(NativeMethods.simpleble_adapter_address));
    /// <summary>Whether the adapter radio is powered on.</summary>
    public bool IsPowered => handle.Query(NativeMethods.simpleble_adapter_is_powered);
    /// <summary>Whether a scan started by this adapter is running.</summary>
    public bool ScanIsActive => handle.Query(NativeMethods.simpleble_adapter_scan_is_active);
    /// <summary>Whether any active backend reports Bluetooth as enabled. Initializes the backends.</summary>
    public static bool IsBluetoothEnabled => NativeCall.Invoke(NativeMethods.simpleble_adapter_is_bluetooth_enabled);

    /// <summary>Initializes backends and adapters. Dispose every returned adapter.</summary>
    public static IReadOnlyList<Adapter> GetAdapters()
    {
        nint list = 0;
        nuint count = NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_adapter_get_adapters(out list, ref error));
        return NativeCall.Collect(list, count, NativeMethods.simpleble_adapter_list_release, (value, index) =>
            new Adapter(NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_adapter_list_get(value, index, ref error))));
    }

    /// <summary>Requests power on. Unsupported native backends may do nothing.</summary>
    public void PowerOn() => handle.Execute(NativeMethods.simpleble_adapter_power_on);
    /// <summary>Requests power off. Unsupported native backends may do nothing.</summary>
    public void PowerOff() => handle.Execute(NativeMethods.simpleble_adapter_power_off);
    /// <summary>Starts scanning without blocking. Results arrive through ScanFound and ScanUpdated.</summary>
    public void ScanStart() => handle.Execute(NativeMethods.simpleble_adapter_scan_start);
    /// <summary>Stops a scan started with ScanStart.</summary>
    public void ScanStop() => handle.Execute(NativeMethods.simpleble_adapter_scan_stop);

    /// <summary>Creates a local GATT host on this adapter. Dispose the returned host after stopping it.</summary>
    public Local.Peripheral CreateLocalPeripheral() => new(handle.Query(NativeMethods.simpleble_adapter_create_local_peripheral));

    /// <summary>Scans synchronously. Positive fractional milliseconds round up.</summary>
    public void ScanFor(TimeSpan duration)
    {
        if (duration.TotalMilliseconds < int.MinValue || duration.TotalMilliseconds > int.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(duration), "Duration must fit a 32-bit millisecond count.");
        int milliseconds = checked((int)Math.Ceiling(duration.TotalMilliseconds));
        handle.Execute((nint value, ref nint error) =>
            NativeMethods.simpleble_adapter_scan_for(value, milliseconds, ref error));
    }

    /// <summary>Runs the blocking native scan on a worker. Cancellation prevents queued work from starting;
    /// once started, the scan runs to completion and its result is observed.</summary>
    public Task ScanForAsync(TimeSpan duration, CancellationToken cancellationToken = default) =>
        Task.Run(() => ScanFor(duration), cancellationToken);

    /// <summary>Returns owned peripheral wrappers. Dispose all entries, including unselected ones.</summary>
    public IReadOnlyList<Peripheral> ScanGetResults() => GetPeripherals(NativeMethods.simpleble_adapter_scan_get_results);

    /// <summary>Returns owned wrappers. Support depends on the native backend.</summary>
    public IReadOnlyList<Peripheral> GetPairedPeripherals() =>
        GetPeripherals(NativeMethods.simpleble_adapter_get_paired_peripherals);

    /// <summary>Returns owned wrappers. Support depends on the native backend.</summary>
    public IReadOnlyList<Peripheral> GetConnectedPeripherals() =>
        GetPeripherals(NativeMethods.simpleble_adapter_get_connected_peripherals);

    private delegate nuint PeripheralsRead(nint adapter, out nint list, ref nint error);
    private IReadOnlyList<Peripheral> GetPeripherals(PeripheralsRead read)
    {
        nint list = 0;
        nuint count = handle.Query((nint value, ref nint error) => read(value, out list, ref error));
        return NativeCall.Collect(list, count, NativeMethods.simpleble_peripheral_list_release, (value, index) =>
            new Peripheral(NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_peripheral_list_get(value, index, ref error))));
    }

    private readonly object eventGate = new();
    private EventHandler? power_on;
    /// <summary>Raised when the adapter powers on. Handlers run on a worker thread.</summary>
    public event EventHandler PoweredOn
    {
        add { lock (eventGate) SetPowerOn(power_on + value); }
        remove { lock (eventGate) SetPowerOn(power_on - value); }
    }
    private void SetPowerOn(EventHandler? handlers)
    {
        callbacks.Set("power_on", handlers is null ? null : _ => { Events.Raise(handlers, this); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_power_on(h, id == 0 ? null : NativeCallbacks.OnSignal, id));
        power_on = handlers;
    }

    private EventHandler? power_off;
    /// <summary>Raised when the adapter powers off. Handlers run on a worker thread.</summary>
    public event EventHandler PoweredOff
    {
        add { lock (eventGate) SetPowerOff(power_off + value); }
        remove { lock (eventGate) SetPowerOff(power_off - value); }
    }
    private void SetPowerOff(EventHandler? handlers)
    {
        callbacks.Set("power_off", handlers is null ? null : _ => { Events.Raise(handlers, this); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_power_off(h, id == 0 ? null : NativeCallbacks.OnSignal, id));
        power_off = handlers;
    }

    private EventHandler? scan_start;
    /// <summary>Raised when a scan starts. Handlers run on a worker thread.</summary>
    public event EventHandler ScanStarted
    {
        add { lock (eventGate) SetScanStart(scan_start + value); }
        remove { lock (eventGate) SetScanStart(scan_start - value); }
    }
    private void SetScanStart(EventHandler? handlers)
    {
        callbacks.Set("scan_start", handlers is null ? null : _ => { Events.Raise(handlers, this); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_scan_start(h, id == 0 ? null : NativeCallbacks.OnSignal, id));
        scan_start = handlers;
    }

    private EventHandler? scan_stop;
    /// <summary>Raised when a scan stops. Handlers run on a worker thread.</summary>
    public event EventHandler ScanStopped
    {
        add { lock (eventGate) SetScanStop(scan_stop + value); }
        remove { lock (eventGate) SetScanStop(scan_stop - value); }
    }
    private void SetScanStop(EventHandler? handlers)
    {
        callbacks.Set("scan_stop", handlers is null ? null : _ => { Events.Raise(handlers, this); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_scan_stop(h, id == 0 ? null : NativeCallbacks.OnSignal, id));
        scan_stop = handlers;
    }

    private EventHandler<PeripheralEventArgs>? scan_found;
    /// <summary>Raised the first time a scan sees a peripheral. Dispose the event's peripheral after use.</summary>
    public event EventHandler<PeripheralEventArgs> ScanFound
    {
        add { lock (eventGate) SetScanFound(scan_found + value); }
        remove { lock (eventGate) SetScanFound(scan_found - value); }
    }
    private void SetScanFound(EventHandler<PeripheralEventArgs>? handlers)
    {
        callbacks.Set("scan_found", handlers is null ? null : value =>
        { Events.Raise(handlers, this, new PeripheralEventArgs((Peripheral)value!)); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_scan_found(h, id == 0 ? null : NativeCallbacks.OnFound, id));
        scan_found = handlers;
    }

    private EventHandler<PeripheralEventArgs>? scan_updated;
    /// <summary>Raised when a known peripheral advertises again. Dispose the event's peripheral after use.</summary>
    public event EventHandler<PeripheralEventArgs> ScanUpdated
    {
        add { lock (eventGate) SetScanUpdated(scan_updated + value); }
        remove { lock (eventGate) SetScanUpdated(scan_updated - value); }
    }
    private void SetScanUpdated(EventHandler<PeripheralEventArgs>? handlers)
    {
        callbacks.Set("scan_updated", handlers is null ? null : value =>
        { Events.Raise(handlers, this, new PeripheralEventArgs((Peripheral)value!)); return null; },
            (h, id) => NativeMethods.simpleble_adapter_set_callback_on_scan_updated(h, id == 0 ? null : NativeCallbacks.OnFound, id));
        scan_updated = handlers;
    }

    /// <summary>Releases this wrapper and its event registrations.</summary>
    public void Dispose()
    {
        callbacks.Dispose();
        lock (eventGate)
        {
            power_on = power_off = scan_start = scan_stop = null;
            scan_found = scan_updated = null;
        }
    }
}
