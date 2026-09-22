using SimpleSharpBLE;

var adapters = Adapter.GetAdapters();
try
{
    var adapter = adapters.Single();
    var found = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
    adapter.ScanFound += (_, args) =>
    {
        using var peripheral = args.Peripheral;
        found.TrySetResult(peripheral.Identifier);
    };
    await adapter.ScanForAsync(TimeSpan.Zero);
    if (await found.Task.WaitAsync(TimeSpan.FromSeconds(5)) != "Plain Peripheral")
        throw new InvalidOperationException("Expected the PLAIN peripheral.");
    Console.WriteLine("PASS: NativeAOT async scan and callback.");
}
finally
{
    foreach (var adapter in adapters) adapter.Dispose();
}
