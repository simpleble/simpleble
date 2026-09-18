#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

Console.WriteLine($"SimpleBLE version: {Utils.Version}");
Console.WriteLine($"Bluetooth enabled: {Adapter.IsBluetoothEnabled}");
var adapters = Adapter.GetAdapters();
try
{
    if (adapters.Count == 0) Console.WriteLine("No adapters found.");
    foreach (var adapter in adapters)
    {
        Console.WriteLine($"Adapter: {adapter.Identifier} [{adapter.Address}]");
        adapter.PoweredOn += (_, _) => Console.WriteLine("Adapter powered on.");
        adapter.PoweredOff += (_, _) => Console.WriteLine("Adapter powered off.");
        Console.WriteLine($"Powered: {adapter.IsPowered}");
        adapter.PowerOff();
        try
        {
            Console.WriteLine($"Powered: {adapter.IsPowered}");
            await Task.Delay(TimeSpan.FromSeconds(5));
        }
        finally
        {
            adapter.PowerOn();
        }
        Console.WriteLine($"Powered: {adapter.IsPowered}");
    }
}
finally
{
    foreach (var adapter in adapters) adapter.Dispose();
}
