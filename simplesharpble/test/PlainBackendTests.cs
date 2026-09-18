using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;
using Xunit;

namespace SimpleSharpBLE.Tests;

public sealed class PlainBackendTests
{
    private static Adapter OpenAdapter()
    {
        var adapters = Adapter.GetAdapters();
        try
        {
            Assert.Single(adapters);
            Assert.Equal("Plain Adapter", adapters[0].Identifier);
            return adapters[0];
        }
        catch
        {
            foreach (var adapter in adapters) adapter.Dispose();
            throw;
        }
    }

    [Fact]
    public void BackendEnumerationTransfersIndependentAdapterOwnership()
    {
        var backends = Backend.GetBackends();
        IReadOnlyList<Adapter>? adapters = null;
        try
        {
            var backend = Assert.Single(backends);
            Assert.Equal("Plain", backend.Identifier);
            Assert.True(backend.IsBluetoothEnabled);
            adapters = backend.Adapters;
            backend.Dispose();
            Assert.Throws<ObjectDisposedException>(() => backend.Identifier);
            Assert.Equal("Plain Adapter", Assert.Single(adapters).Identifier);
        }
        finally
        {
            if (adapters is not null) foreach (var adapter in adapters) adapter.Dispose();
            foreach (var backend in backends) backend.Dispose();
        }
    }

    [Fact]
    public void ScanStateAndStringsCrossTheNativeBoundary()
    {
        using var adapter = OpenAdapter();
        Assert.True(Adapter.IsBluetoothEnabled);
        Assert.Equal("AA:BB:CC:DD:EE:FF", adapter.Address);
        Assert.True(adapter.IsPowered);
        adapter.ScanStart();
        Assert.True(adapter.ScanIsActive);
        adapter.ScanStop();
        Assert.False(adapter.ScanIsActive);
        adapter.ScanFor(TimeSpan.Zero);
        Assert.False(adapter.ScanIsActive);
    }

    [Fact]
    public void PeripheralOutlivesAdapterAndSupportsConnectionState()
    {
        using var adapter = OpenAdapter();
        var peripherals = adapter.ScanGetResults();
        try
        {
            var peripheral = Assert.Single(peripherals);
            adapter.Dispose();
            Assert.Equal("Plain Peripheral", peripheral.Identifier);
            Assert.Equal("11:22:33:44:55:66", peripheral.Address);
            Assert.Equal(BluetoothAddressType.Public, peripheral.AddressType);
            Assert.Equal(-60, peripheral.Rssi);
            Assert.Equal(5, peripheral.TxPower);
            Assert.True(peripheral.IsConnectable);
            Assert.False(peripheral.IsConnected);
            Assert.Equal(0, peripheral.Mtu);
            peripheral.Connect();
            Assert.True(peripheral.IsConnected);
            Assert.True(peripheral.IsPaired);
            Assert.Equal(247, peripheral.Mtu);
            peripheral.Disconnect();
            Assert.False(peripheral.IsConnected);
            peripheral.Unpair();
            Assert.False(peripheral.IsPaired);
            peripheral.Dispose();
            peripheral.Dispose();
            Assert.Throws<ObjectDisposedException>(() => peripheral.Connect());
        }
        finally
        {
            foreach (var peripheral in peripherals) peripheral.Dispose();
        }
    }

    [Fact]
    public void InvalidDurationsFailBeforeStartingScan()
    {
        using var adapter = OpenAdapter();
        Assert.Throws<ArgumentOutOfRangeException>(() => adapter.ScanFor(TimeSpan.FromTicks(-1)));
        Assert.Throws<ArgumentOutOfRangeException>(() => adapter.ScanFor(TimeSpan.MaxValue));
        Assert.False(adapter.ScanIsActive);
        adapter.ScanFor(TimeSpan.FromTicks(1));
        Assert.False(adapter.ScanIsActive);
    }

    [Fact]
    public void ErrorsPreserveDetailsAndDoNotContaminateLaterCalls()
    {
        for (int i = 0; i < 100; i++)
        {
            var exception = Assert.Throws<BleException>(() => NativeCall.Invoke((ref nint error) =>
                NativeMethods.simpleble_backend_get_handle(nuint.MaxValue, ref error)));
            Assert.Equal(BleErrorCode.InvalidArgument, exception.Code);
            Assert.Contains("index", exception.Message);
        }
        using var adapter = OpenAdapter();
        Assert.True(adapter.IsPowered);
    }

    [Fact]
    public void PartialEnumerationDisposesAlreadyCreatedWrappers()
    {
        using var adapter = OpenAdapter();
        Assert.Throws<InvalidOperationException>(() => NativeCall.Collect<Adapter>(2, index =>
            index == 0 ? adapter : throw new InvalidOperationException("Simulated retrieval failure")));
        Assert.Throws<ObjectDisposedException>(() => adapter.Identifier);
    }

    [Fact]
    public async Task InFlightCallRetainsHandleAcrossConcurrentDisposal()
    {
        var value = NativeCall.Invoke((ref nint error) => NativeMethods.simpleble_adapter_get_handle(0, ref error));
        using var handle = new NativeHandle(value, HandleKind.Adapter);
        using var entered = new ManualResetEventSlim();
        using var resume = new ManualResetEventSlim();
        var call = Task.Run(() => handle.Query((nint pointer, ref nint error) =>
        {
            entered.Set();
            if (!resume.Wait(TimeSpan.FromSeconds(10))) throw new TimeoutException();
            return NativeMethods.simpleble_adapter_identifier(pointer, ref error);
        }));
        try
        {
            Assert.True(entered.Wait(TimeSpan.FromSeconds(10)));
            handle.Dispose();
            Assert.Throws<ObjectDisposedException>(() => handle.Query(NativeMethods.simpleble_adapter_is_powered));
        }
        finally
        {
            resume.Set();
        }
        Assert.Equal("Plain Adapter", NativeCall.String(await call));
        Assert.Throws<ObjectDisposedException>(() => handle.Query(NativeMethods.simpleble_adapter_is_powered));
    }
}
