#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

using var adapter = Select(Adapter.GetAdapters(),
    a => $"{a.Identifier} [{a.Address}]", "Select an adapter");
if (adapter is null) return;

adapter.ScanStarted += (_, _) => Console.WriteLine("Scan started.");
adapter.ScanStopped += (_, _) => Console.WriteLine("Scan stopped.");
adapter.ScanFor(TimeSpan.FromSeconds(5));
using var peripheral = Select(adapter.ScanGetResults(),
    p => $"{p.Identifier} [{p.Address}] Connectable: {p.IsConnectable}", "Select a device");
if (peripheral is null) return;
if (!peripheral.IsConnectable)
{
    Console.WriteLine("Device is not connectable.");
    return;
}

peripheral.Connect();
try
{
    var target = SelectCharacteristic(peripheral, c => c.CanNotify);
    if (target is null) return;
    peripheral.Notify(target.Value.Service, target.Value.Characteristic,
        bytes => Console.WriteLine($"Received: {Convert.ToHexString(bytes)}"));
    try
    {
        await Task.Delay(TimeSpan.FromSeconds(5));
    }
    finally
    {
        peripheral.Unsubscribe(target.Value.Service, target.Value.Characteristic);
    }
}
finally
{
    peripheral.Disconnect();
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

static (string Service, string Characteristic)? SelectCharacteristic(
    Peripheral peripheral, Func<Characteristic, bool> matches)
{
    var choices = peripheral.Services.SelectMany(service => service.Characteristics
        .Where(matches).Select(c => (Service: service.Uuid, Characteristic: c.Uuid))).ToArray();
    if (choices.Length == 0)
    {
        Console.WriteLine("No matching characteristics found.");
        return null;
    }
    for (int i = 0; i < choices.Length; i++)
        Console.WriteLine($"[{i}] {choices[i].Service} {choices[i].Characteristic}");
    while (true)
    {
        Console.Write($"Select a characteristic (0-{choices.Length - 1}): ");
        string? input = Console.ReadLine();
        if (input is null) return null;
        if (int.TryParse(input, out int index) && index >= 0 && index < choices.Length)
            return choices[index];
    }
}
