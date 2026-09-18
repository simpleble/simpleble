#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

const string serviceUuid = "12345678-1234-5678-1234-56789abcdef0";
const string characteristicUuid = "12345678-1234-5678-1234-56789abcdef1";

using var adapter = Select(Adapter.GetAdapters(),
    a => $"{a.Identifier} [{a.Address}]", "Select an adapter");
if (adapter is null) return;

using var peripheral = adapter.CreateLocalPeripheral();
peripheral.AddAdvertisedService(serviceUuid);
using var service = peripheral.AddService(serviceUuid);
using var characteristic = service.AddCharacteristic(characteristicUuid,
    SimpleSharpBLE.Local.CharacteristicCapability.Read |
    SimpleSharpBLE.Local.CharacteristicCapability.WriteRequest |
    SimpleSharpBLE.Local.CharacteristicCapability.WriteCommand |
    SimpleSharpBLE.Local.CharacteristicCapability.Notify |
    SimpleSharpBLE.Local.CharacteristicCapability.Indicate);
characteristic.Value = "ready"u8.ToArray();
characteristic.Written += (_, e) =>
{
    Console.WriteLine($"Write: {Convert.ToHexString(e.Value)}");
    characteristic.Value = e.Value;
};
characteristic.Subscribed += (_, _) => Console.WriteLine("Client subscribed.");
characteristic.Unsubscribed += (_, _) => Console.WriteLine("Client unsubscribed.");
peripheral.ClientConnected += (_, e) => Console.WriteLine($"Client connected: {e.Address}");
peripheral.ClientDisconnected += (_, e) => Console.WriteLine($"Client disconnected: {e.Address}");

var stopped = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
ConsoleCancelEventHandler onCancel = (_, e) =>
{
    e.Cancel = true;
    stopped.TrySetResult();
};
Console.CancelKeyPress += onCancel;
try
{
    peripheral.Start();
    try
    {
        Console.WriteLine("Advertising. Press Ctrl+C to stop.");
        await stopped.Task;
    }
    finally
    {
        peripheral.Stop();
    }
}
finally
{
    Console.CancelKeyPress -= onCancel;
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
