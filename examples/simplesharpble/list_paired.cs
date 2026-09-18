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
        var peripherals = adapter.GetPairedPeripherals();
        try
        {
            foreach (var peripheral in peripherals)
                Console.WriteLine($"  {peripheral.Identifier} [{peripheral.Address}]");
        }
        finally
        {
            foreach (var peripheral in peripherals) peripheral.Dispose();
        }
    }
}
finally
{
    foreach (var adapter in adapters) adapter.Dispose();
}
