#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

using var adapter = Select(Adapter.GetAdapters(),
    a => $"{a.Identifier} [{a.Address}]", "Select an adapter");
if (adapter is null) return;

adapter.ScanStarted += (_, _) => Console.WriteLine("Scan started.");
adapter.ScanStopped += (_, _) => Console.WriteLine("Scan stopped.");
adapter.ScanFound += (_, e) =>
{
    using var peripheral = e.Peripheral;
    Console.WriteLine($"Found: {peripheral.Identifier} [{peripheral.Address}] {peripheral.Rssi} dBm");
};
adapter.ScanUpdated += (_, e) =>
{
    using var peripheral = e.Peripheral;
    Console.WriteLine($"Updated: {peripheral.Identifier} [{peripheral.Address}] {peripheral.Rssi} dBm");
};
adapter.ScanFor(TimeSpan.FromSeconds(2));
var peripherals = adapter.ScanGetResults();
try
{
    Console.WriteLine("Scan complete.");
    foreach (var peripheral in peripherals)
    {
        Console.WriteLine($"{peripheral.Identifier} [{peripheral.Address}] {peripheral.Rssi} dBm");
        Console.WriteLine($"  Connectable: {peripheral.IsConnectable}");
        Console.WriteLine($"  Tx power: {peripheral.TxPower} dBm");
        Console.WriteLine($"  Address type: {peripheral.AddressType}");
        foreach (var service in peripheral.Services)
            Console.WriteLine($"  Service: {service.Uuid}, data: {Convert.ToHexString(service.Data)}");
        foreach (var (id, data) in peripheral.ManufacturerData)
            Console.WriteLine($"  Manufacturer: {id}, data: {Convert.ToHexString(data)}");
    }
}
finally
{
    foreach (var peripheral in peripherals) peripheral.Dispose();
}

static T? Select<T>(IReadOnlyList<T> items, Func<T, string> describe, string prompt)
    where T : class, IDisposable
{
    T? selected = null;
    try
    {
        if (items.Count == 0)
        {
            Console.WriteLine("No matching devices found.");
            return null;
        }
        for (int i = 0; i < items.Count; i++)
            Console.WriteLine($"[{i}] {describe(items[i])}");
        while (true)
        {
            Console.Write($"{prompt} (0-{items.Count - 1}): ");
            string? input = Console.ReadLine();
            if (input is null) return null;
            if (int.TryParse(input, out int index) && index >= 0 && index < items.Count)
                return selected = items[index];
        }
    }
    finally
    {
        foreach (var item in items)
            if (!ReferenceEquals(item, selected)) item.Dispose();
    }
}
