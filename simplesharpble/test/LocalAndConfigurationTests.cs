using SimpleSharpBLE.Local;
using Xunit;

namespace SimpleSharpBLE.Tests;

public sealed class LocalAndConfigurationTests
{
    private static byte[] Read(Local.Characteristic characteristic)
    {
        byte[] result = new byte[65537];
        var length = characteristic.handle.Query((nint h, ref nint e) => NativeFixture.test_read_local(h, result, (nuint)result.Length));
        return result[..checked((int)length)];
    }

    [Fact]
    public void ReadRegistrationIsSharedAcrossEnumeratedWrappers()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var service = host.AddService(GattTests.Uuid);
        using var first = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        using var second = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        first.ReadHandler = () => [1];
        second.ReadHandler = () => [2];
        using var serviceAlias = host.Services.Single();
        var aliases = serviceAlias.Characteristics;
        try
        {
            Assert.Equal(new byte[] { 1 }, Read(aliases[0]));
            Assert.Equal(new byte[] { 2 }, Read(aliases[1]));
            aliases[0].ReadHandler = () => [3];
            first.Dispose();
            Assert.Equal(new byte[] { 3 }, Read(aliases[0]));
            Assert.Equal(new byte[] { 2 }, Read(aliases[1]));
            aliases[0].ReadHandler = null;
            aliases[0].Value = [4];
            Assert.Equal(new byte[] { 4 }, Read(aliases[0]));
        }
        finally { foreach (var alias in aliases) alias.Dispose(); }
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task RetiringReadRegistrationWaitsForActiveRead(bool replace)
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        using var alias = service.Characteristics.Single();
        using var entered = new ManualResetEventSlim();
        using var finish = new ManualResetEventSlim();
        characteristic.Value = [7];
        characteristic.ReadHandler = () =>
        {
            entered.Set();
            if (!finish.Wait(TimeSpan.FromSeconds(10))) throw new TimeoutException();
            return Enumerable.Repeat((byte)42, 65537).ToArray();
        };
        var readTask = Task.Run(() => Read(alias));
        Assert.True(entered.Wait(TimeSpan.FromSeconds(5)));
        var disposeTask = Task.Run(() =>
        {
            if (replace) alias.ReadHandler = () => [7];
            else characteristic.Dispose();
        });
        try { Assert.NotSame(disposeTask, await Task.WhenAny(disposeTask, Task.Delay(100))); }
        finally { finish.Set(); }
        Assert.Equal(Enumerable.Repeat((byte)42, 65537), await readTask.WaitAsync(TimeSpan.FromSeconds(5)));
        await disposeTask.WaitAsync(TimeSpan.FromSeconds(5));
        Assert.Equal(new byte[] { 7 }, Read(alias));
    }

    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
    private static WeakReference RegisterTemporaryReader(Local.Service service)
    {
        var characteristic = service.Characteristics.Single();
        characteristic.ReadHandler = () => characteristic.Value;
        Assert.Equal(new byte[] { 8 }, Read(characteristic));
        return new WeakReference(characteristic);
    }

    [Fact]
    public void CollectedReadOwnerClearsCallbackAndReleasesNativeHandle()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        characteristic.Value = [8];
        var owner = RegisterTemporaryReader(service);
        GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
        Assert.False(owner.IsAlive);
        characteristic.Value = [9];
        Assert.Equal(new byte[] { 9 }, Read(characteristic));
    }

    [Fact]
    public void RemovedServicesHaveIndependentReadRegistrations()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var oldService = host.AddService(GattTests.Uuid);
        using var oldCharacteristic = oldService.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        oldCharacteristic.ReadHandler = () => [1];
        host.RemoveAllServices();
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        characteristic.ReadHandler = () => [2];
        oldCharacteristic.Dispose();
        Assert.Equal(new byte[] { 2 }, Read(characteristic));
    }

    [Fact]
    public void ReadHandlerCannotDisposeOrReplaceItself()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
        characteristic.ReadHandler = () =>
        {
            Assert.Throws<InvalidOperationException>(characteristic.Dispose);
            Assert.Throws<InvalidOperationException>(() => characteristic.ReadHandler = null);
            return [3];
        };
        Assert.Equal(new byte[] { 3 }, Read(characteristic));
    }

    [Fact]
    public void PlainBackendHostsLocalGattServices()
    {
        var adapters = Adapter.GetAdapters();
        try
        {
            using var host = adapters[0].CreateLocalPeripheral();
            using var service = host.AddService(GattTests.Uuid);
            using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
            byte[] expected = [0, 255, 128];
            characteristic.Value = expected;
            host.Start();
            Assert.True(host.IsStarted);
            Assert.True(host.IsAdvertising);
            adapters[0].ScanFor(TimeSpan.FromMilliseconds(250));
            var peers = adapters[0].ScanGetResults();
            try
            {
                var peer = Assert.Single(peers, p => p.Identifier == "Plain Adapter Peripheral");
                peer.Connect();
                Assert.Equal(expected, peer.Read(GattTests.Uuid, GattTests.Uuid));
                host.Stop();
                Assert.False(peer.IsConnected);
            }
            finally { foreach (var peer in peers) peer.Dispose(); }
        }
        finally { foreach (var adapter in adapters) adapter.Dispose(); }
    }

    [Fact]
    public void PlainHostAdvertisesManufacturerDataWithoutConnections()
    {
        var adapters = Adapter.GetAdapters();
        try
        {
            using var host = adapters[0].CreateLocalPeripheral();
            host.SetAdvertisedManufacturerData(0x004c, [0x02]);
            host.SetAdvertisedManufacturerData(0x004c, [0x02, 0x15]);
            host.SetConnectable(false);
            host.Start();
            adapters[0].ScanFor(TimeSpan.FromMilliseconds(250));
            var peers = adapters[0].ScanGetResults();
            try
            {
                var peer = Assert.Single(peers, p => p.Identifier == "Plain Adapter Peripheral");
                Assert.Equal(new byte[] { 0x02, 0x15 }, Assert.Single(peer.ManufacturerData).Value);
                Assert.False(peer.IsConnectable);
                host.Stop();
            }
            finally { foreach (var peer in peers) peer.Dispose(); }
        }
        finally { foreach (var adapter in adapters) adapter.Dispose(); }
    }

    [Fact]
    public async Task PlainHostAliasesFollowUuidsNotPositions()
    {
        var adapters = Adapter.GetAdapters();
        try
        {
            using var host = adapters[0].CreateLocalPeripheral();
            using var service = host.AddService(GattTests.Uuid);
            using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.Read);
            characteristic.Value = [9];
            // PLAIN lists services by UUID, so this one moves ahead of the first.
            using var first = host.AddService("00000001-0000-1000-8000-00805f9b34fb");
            characteristic.ReadHandler = () => [1];
            var services = host.Services;
            var aliases = services.Single(s => s.Uuid == GattTests.Uuid).Characteristics;
            try
            {
                aliases.Single().ReadHandler = () => [2];
                // Disposing the original must not clear the alias's replacement handler.
                characteristic.Dispose();
                host.Start();
                var found = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
                EventHandler<PeripheralEventArgs> onFound = (_, args) =>
                {
                    try
                    {
                        if (args.Peripheral.Identifier == "Plain Adapter Peripheral") found.TrySetResult();
                    }
                    finally { args.Peripheral.Dispose(); }
                };
                adapters[0].ScanFound += onFound;
                try
                {
                    adapters[0].ScanStart();
                    await found.Task.WaitAsync(TimeSpan.FromSeconds(5));
                }
                finally
                {
                    adapters[0].ScanStop();
                    adapters[0].ScanFound -= onFound;
                }
                var peers = adapters[0].ScanGetResults();
                try
                {
                    var peer = Assert.Single(peers, p => p.Identifier == "Plain Adapter Peripheral");
                    peer.Connect();
                    Assert.Equal(new byte[] { 2 }, peer.Read(GattTests.Uuid, GattTests.Uuid));
                    host.Stop();
                }
                finally { foreach (var peer in peers) peer.Dispose(); }
            }
            finally
            {
                foreach (var alias in aliases) alias.Dispose();
                foreach (var alias in services) alias.Dispose();
            }
        }
        finally { foreach (var adapter in adapters) adapter.Dispose(); }
    }

    [Fact]
    public async Task LocalHostAndDynamicReadResultsRoundTripThroughCAbi()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        host.AddAdvertisedService(new[] { GattTests.Uuid });
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid,
            CharacteristicCapability.Read | CharacteristicCapability.WriteRequest | CharacteristicCapability.Notify);
        Assert.Equal(GattTests.Uuid, service.Uuid);
        Assert.Equal(GattTests.Uuid, characteristic.Uuid);
        Assert.True(characteristic.Capabilities.HasFlag(CharacteristicCapability.Notify));
        var connected = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
        host.ClientConnected += (_, args) => connected.TrySetResult(args.Address);
        host.Start();
        Assert.True(host.IsStarted); Assert.True(host.IsAdvertising);
        Assert.Equal("fixture-client", await connected.Task.WaitAsync(TimeSpan.FromSeconds(5)));
        Assert.Throws<BleException>(() => host.RemoveAllServices());

        foreach (int length in new[] { 0, 1, 257 })
        {
            byte[] expected = Enumerable.Range(0, length).Select(x => (byte)x).ToArray();
            characteristic.Value = expected;
            Assert.Equal(expected, characteristic.Value);
            characteristic.ReadHandler = () => expected;
            Assert.Equal(expected, Read(characteristic));
        }
        characteristic.ReadHandler = null;
        characteristic.Value = [];
        Assert.Empty(characteristic.Value);
        host.Stop(); Assert.False(host.IsStarted);
        host.Dispose(); service.Dispose();
        Assert.Equal(GattTests.Uuid, characteristic.Uuid);
    }

    [Fact]
    public async Task LocalWriteAndSubscriptionEventsCopyTheirValues()
    {
        using var host = new Local.Peripheral(NativeFixture.test_local());
        using var service = host.AddService(GattTests.Uuid);
        using var characteristic = service.AddCharacteristic(GattTests.Uuid, CharacteristicCapability.WriteRequest);
        var write = new TaskCompletionSource<byte[]>(TaskCreationOptions.RunContinuationsAsynchronously);
        var subscribe = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var unsubscribe = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        characteristic.Written += (_, args) => write.TrySetResult(args.Value);
        characteristic.Subscribed += (_, _) => subscribe.TrySetResult();
        characteristic.Unsubscribed += (_, _) => unsubscribe.TrySetResult();
        byte[] bytes = [0, 1, 255];
        characteristic.handle.Query((nint h, ref nint e) => { NativeFixture.test_write_local(h, bytes, (nuint)bytes.Length); return 0; });
        Assert.Equal(bytes, await write.Task.WaitAsync(TimeSpan.FromSeconds(5)));
        await subscribe.Task.WaitAsync(TimeSpan.FromSeconds(5)); await unsubscribe.Task.WaitAsync(TimeSpan.FromSeconds(5));
    }

    [Fact]
    public void ConfigurationRoundTripsAndResets()
    {
        // Test native config access in isolation. No backend operation occurs
        // between changing and restoring these process-wide settings.
        Config.ResetAll();
        try
        {
            bool interactivePairing = Config.SimpleBluez.InteractivePairing;
            Config.SimpleBluez.InteractivePairing = !interactivePairing;
            Config.SimpleBluez.UseSystemBus = false;
            Config.SimpleBluez.ConnectionTimeout = TimeSpan.FromMilliseconds(1234);
            Config.SimpleBluez.DisconnectionTimeout = TimeSpan.FromMilliseconds(4321);
            Config.WinRT.UseDeferredDisconnect = false;
            Config.Android.ConnectionPriority = AndroidConnectionPriority.High;
            Config.Dongl.AutoUpdate = false; Config.Dongl.ForceUpdate = true;
            Assert.False(Config.SimpleBluez.UseSystemBus);
            Assert.Equal(!interactivePairing, Config.SimpleBluez.InteractivePairing);
            Assert.Equal(TimeSpan.FromMilliseconds(1234), Config.SimpleBluez.ConnectionTimeout);
            Assert.Equal(TimeSpan.FromMilliseconds(4321), Config.SimpleBluez.DisconnectionTimeout);
            Assert.False(Config.WinRT.UseDeferredDisconnect);
            Assert.Equal(AndroidConnectionPriority.High, Config.Android.ConnectionPriority);
            Assert.True(Config.Dongl.ForceUpdate);
            Assert.Throws<ArgumentOutOfRangeException>(() => Config.SimpleBluez.ConnectionTimeout = TimeSpan.FromSeconds(-1));
            Assert.Throws<ArgumentOutOfRangeException>(() => Config.Android.ConnectionPriority = (AndroidConnectionPriority)123);
            Assert.NotEmpty(Utils.Version);
        }
        finally { Config.ResetAll(); }
    }

    [Fact]
    public async Task LoggingCopiesUtf8AndCanBeCleared()
    {
        var done = new TaskCompletionSource<LogMessage>(TaskCreationOptions.RunContinuationsAsynchronously);
        var previous = Logging.Level;
        try
        {
            Logging.Level = LogLevel.Verbose;
            Logging.SetCallback(message => done.TrySetResult(message));
            NativeFixture.test_log();
            var message = await done.Task.WaitAsync(TimeSpan.FromSeconds(5));
            Assert.Equal("message é", message.Message);
            Assert.Equal(12u, message.Line);
            Logging.SetCallback(null);
            Assert.False(Logging.HasCallback);
        }
        finally { Logging.Level = previous; Logging.LogDefaultStdout(); }
    }

    [Fact]
    public void PairingCallbacksRejectUnsupportedBackendsWithoutLeakingRegistration()
    {
        using var peripheral = new SimpleSharpBLE.Peripheral(NativeFixture.test_remote());
        Assert.Equal(BleErrorCode.OperationNotSupported,
            Assert.Throws<BleException>(() => Advanced.Dongl.SetPasskeyRequestCallback(peripheral, () => "001234")).Code);

        // PLAIN has no BlueZ adapter, and other platforms lack the Linux functions.
        var adapters = Adapter.GetAdapters();
        try
        {
            var adapter = adapters[0];
            void Unsupported(Action register)
            {
                if (Utils.OperatingSystem == NativeOperatingSystem.Linux)
                    Assert.Equal(BleErrorCode.OperationNotSupported, Assert.Throws<BleException>(register).Code);
                else
                    Assert.Throws<PlatformNotSupportedException>(register);
            }
            Unsupported(() => Advanced.Linux.SetPasskeyRequestCallback(adapter, _ => "001234"));
            Unsupported(() => Advanced.Linux.SetPasskeyDisplayCallback(adapter, (_, _) => { }));
            Unsupported(() => Advanced.Linux.SetNumericComparisonCallback(adapter, (_, _) => true));
            Unsupported(() => Advanced.Linux.SetPairingAuthorizationCallback(adapter, _ => true));
        }
        finally { foreach (var adapter in adapters) adapter.Dispose(); }
    }

}
