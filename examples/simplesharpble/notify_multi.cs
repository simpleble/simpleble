#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

using var adapter = Select(Adapter.GetAdapters(),
    a => $"{a.Identifier} [{a.Address}]", "Select an adapter");
if (adapter is null) return;

var connections = new List<(Peripheral Peripheral, string Service, string Characteristic)>();
int printAllowed = 0;
try
{
    for (int i = 0; i < 2; i++)
    {
        Console.WriteLine($"Choose device {i + 1}:");
        adapter.ScanFor(TimeSpan.FromSeconds(5));
        var peripheral = Select(adapter.ScanGetResults(),
            p => $"{p.Identifier} [{p.Address}]", "Select a device");
        if (peripheral is null) return;
        bool retained = false;
        try
        {
            if (connections.Any(c => c.Peripheral.Address == peripheral.Address))
            {
                Console.WriteLine("Select a different device.");
                return;
            }
            peripheral.Connect();
            try
            {
                var target = SelectCharacteristic(peripheral, c => c.CanNotify);
                if (target is null) return;
                int device = i + 1;
                peripheral.Notify(target.Value.Service, target.Value.Characteristic, bytes =>
                {
                    if (Volatile.Read(ref printAllowed) != 0)
                        Console.WriteLine($"Peripheral {device}: {Convert.ToHexString(bytes)}");
                });
                connections.Add((peripheral, target.Value.Service, target.Value.Characteristic));
                retained = true;
            }
            finally
            {
                if (!retained) peripheral.Disconnect();
            }
        }
        finally
        {
            if (!retained) peripheral.Dispose();
        }
    }
    Volatile.Write(ref printAllowed, 1);
    await Task.Delay(TimeSpan.FromSeconds(5));
    CloseLast();
    await Task.Delay(TimeSpan.FromSeconds(3));
}
finally
{
    try { if (connections.Count > 0) CloseLast(); }
    finally
    {
        if (connections.Count > 0) CloseLast();
    }
}

void CloseLast()
{
    var connection = connections[^1];
    connections.RemoveAt(connections.Count - 1);
    using var peripheral = connection.Peripheral;
    try { peripheral.Unsubscribe(connection.Service, connection.Characteristic); }
    finally { peripheral.Disconnect(); }
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
