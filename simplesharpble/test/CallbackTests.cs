using SimpleSharpBLE.Internal;
using SimpleSharpBLE.Internal.Interop;
using Xunit;

namespace SimpleSharpBLE.Tests;

public sealed class CallbackTests
{
    [Fact]
    public async Task AsyncConveniencesObserveResultsAndPreStartCancellation()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        using var cancellation = new CancellationTokenSource();
        cancellation.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(() => peripheral.ConnectAsync(cancellation.Token));
        Assert.False(peripheral.IsConnected);
        await peripheral.ConnectAsync(); Assert.True(peripheral.IsConnected);
        await peripheral.DisconnectAsync(); Assert.False(peripheral.IsConnected);
    }

    [Fact]
    public async Task PlainNativeNotificationWorkerCanBeUnsubscribedFromManagedHandler()
    {
        var adapters = Adapter.GetAdapters();
        IReadOnlyList<Peripheral>? peripherals = null;
        try
        {
            await adapters[0].ScanForAsync(TimeSpan.Zero);
            peripherals = adapters[0].ScanGetResults();
            var peripheral = peripherals[0];
            peripheral.Connect();
            var received = Completion<byte[]>();
            peripheral.Notify(GattTests.Uuid, GattTests.Uuid, data =>
            {
                peripheral.Unsubscribe(GattTests.Uuid, GattTests.Uuid);
                received.TrySetResult(data);
            });
            Assert.Equal("Hello from notify", System.Text.Encoding.UTF8.GetString(await received.Task.WaitAsync(TimeSpan.FromSeconds(8))));
            peripheral.Disconnect();
        }
        finally
        {
            if (peripherals is not null) foreach (var peripheral in peripherals) peripheral.Dispose();
            foreach (var adapter in adapters) adapter.Dispose();
        }
    }
    private static TaskCompletionSource<T> Completion<T>() => new(TaskCreationOptions.RunContinuationsAsynchronously);
    [Fact]
    public async Task EventsOnOneWrapperPreserveNativeArrivalOrder()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        var order = new List<string>();
        var done = Completion<bool>();
        peripheral.Connected += (_, _) => order.Add("connected");
        peripheral.Disconnected += (_, _) => { order.Add("disconnected"); done.TrySetResult(true); };
        peripheral.Connect(); peripheral.Disconnect();
        await done.Task.WaitAsync(TimeSpan.FromSeconds(5));
        Assert.Equal(new[] { "connected", "disconnected" }, order);
    }
    [Fact]
    public async Task ScanEventsTransferOwnedPeripheralsAndContainHandlerExceptions()
    {
        var adapters = Adapter.GetAdapters();
        try
        {
            var adapter = Assert.Single(adapters);
            var found = Completion<Peripheral>(); var error = Completion<Exception>();
            EventHandler<CallbackErrorEventArgs> report = (_, args) => error.TrySetResult(args.Exception);
            CallbackErrors.Unhandled += report;
            try
            {
                adapter.ScanFound += (_, _) => throw new InvalidOperationException("handler");
                adapter.ScanFound += (_, args) => found.TrySetResult(args.Peripheral);
                GC.Collect(); GC.WaitForPendingFinalizers();
                adapter.ScanStart();
                using var peripheral = await found.Task.WaitAsync(TimeSpan.FromSeconds(5));
                Assert.Equal("handler", (await error.Task.WaitAsync(TimeSpan.FromSeconds(5))).Message);
                adapter.ScanStop(); adapter.Dispose();
                Assert.Equal("Plain Peripheral", peripheral.Identifier);
            }
            finally { CallbackErrors.Unhandled -= report; }
        }
        finally { foreach (var adapter in adapters) adapter.Dispose(); }
    }

    [Fact]
    public async Task NotificationsCopyPayloadAndCanUnsubscribeInsideHandler()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        peripheral.Connect();
        var done = Completion<byte[]>();
        peripheral.Notify(GattTests.Uuid, GattTests.Uuid, data =>
        {
            peripheral.Unsubscribe(GattTests.Uuid, GattTests.Uuid);
            done.TrySetResult(data);
        });
        GC.Collect(); GC.WaitForPendingFinalizers();
        peripheral.handle.Query((nint h, ref nint e) => { NativeFixture.test_emit(h); return 0; });
        Assert.Equal(257, (await done.Task.WaitAsync(TimeSpan.FromSeconds(5))).Length);
    }

    [Fact]
    public async Task NewWrapperRegistrationSurvivesOldWrapperDisposal()
    {
        using var first = new Peripheral(NativeFixture.test_remote());
        using var second = new Peripheral(first.handle.Query((nint h, ref nint e) => NativeFixture.test_clone_remote(h)));
        first.Connect();
        first.Notify(GattTests.Uuid, GattTests.Uuid, _ => throw new Exception("Replaced callback must not run"));
        var received = Completion<byte[]>();
        second.Indicate(GattTests.Uuid, GattTests.Uuid, data => received.TrySetResult(data));
        first.Dispose();
        second.handle.Query((nint h, ref nint e) => { NativeFixture.test_emit(h); return 0; });
        Assert.Equal(257, (await received.Task.WaitAsync(TimeSpan.FromSeconds(5))).Length);
        second.Unsubscribe(GattTests.Uuid, GattTests.Uuid);
    }

    [Fact]
    public void ExpiredTokensDropLatePayloadsAndOwnedScanHandles()
    {
        int calls = 0;
        using var slot = new CallbackSlot(_ => { Interlocked.Increment(ref calls); return null; });
        nint token = slot.Id;
        slot.Dispose();
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        CallbackSlot.Post(token, peripheral, peripheral.Dispose);
        NativeCallbacks.OnSignal(0, token);
        Assert.Equal(0, calls);
        Assert.Throws<ObjectDisposedException>(() => peripheral.Identifier);
        Assert.Null(CallbackSlot.Find(token));
    }

    [Fact]
    public async Task DisposeDropsQueuedWorkWithoutDeadlockingTheRunningHandler()
    {
        var started = Completion<bool>(); var finished = Completion<bool>();
        using var release = new ManualResetEventSlim();
        int dropped = 0;
        using var slot = new CallbackSlot(_ =>
        {
            started.TrySetResult(true);
            if (!release.Wait(TimeSpan.FromSeconds(5))) throw new TimeoutException();
            finished.TrySetResult(true); return null;
        });
        CallbackSlot.Post(slot.Id);
        await started.Task.WaitAsync(TimeSpan.FromSeconds(5));
        CallbackSlot.Post(slot.Id, drop: () => Interlocked.Increment(ref dropped));
        slot.Dispose();
        release.Set();
        await finished.Task.WaitAsync(TimeSpan.FromSeconds(5));
        Assert.Equal(1, dropped);
    }

    [Fact]
    public async Task ConnectionEventsAndRepeatedSubscriptionChangesWork()
    {
        using var peripheral = new Peripheral(NativeFixture.test_remote());
        var connected = Completion<bool>(); var disconnected = Completion<bool>();
        peripheral.Connected += (_, _) => connected.TrySetResult(true);
        peripheral.Disconnected += (_, _) => disconnected.TrySetResult(true);
        peripheral.Connect(); await connected.Task.WaitAsync(TimeSpan.FromSeconds(5));
        for (int i = 0; i < 50; i++)
        {
            peripheral.Notify(GattTests.Uuid, GattTests.Uuid, _ => { });
            peripheral.Unsubscribe(GattTests.Uuid, GattTests.Uuid);
        }
        peripheral.Disconnect(); await disconnected.Task.WaitAsync(TimeSpan.FromSeconds(5));
    }
}
