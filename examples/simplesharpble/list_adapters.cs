#:project ../../simplesharpble
#:property PublishAot=false

using SimpleSharpBLE;

Console.WriteLine($"SimpleBLE version: {Utils.Version}");
var backends = Backend.GetBackends();
try
{
    if (backends.Count == 0) Console.WriteLine("No backends found.");
    foreach (var backend in backends)
    {
        Console.WriteLine($"Backend: {backend.Identifier}");
        PrintAdapters(backend.Adapters);
    }
}
finally
{
    foreach (var backend in backends) backend.Dispose();
}
Console.WriteLine("Consolidated adapter list:");
PrintAdapters(Adapter.GetAdapters());

static void PrintAdapters(IReadOnlyList<Adapter> adapters)
{
    try
    {
        if (adapters.Count == 0) Console.WriteLine("No adapters found.");
        foreach (var adapter in adapters)
            Console.WriteLine($"  {adapter.Identifier} [{adapter.Address}]");
    }
    finally
    {
        foreach (var adapter in adapters) adapter.Dispose();
    }
}
