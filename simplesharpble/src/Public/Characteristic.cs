namespace SimpleSharpBLE;

/// <summary>A managed snapshot of a remote characteristic's capabilities and descriptors.</summary>
public sealed record Characteristic(string Uuid, bool CanRead, bool CanWriteRequest,
    bool CanWriteCommand, bool CanNotify, bool CanIndicate, IReadOnlyList<Descriptor> Descriptors)
{
    public IReadOnlyList<string> Capabilities => new[]
    {
        CanRead ? "read" : null, CanWriteRequest ? "write_request" : null,
        CanWriteCommand ? "write_command" : null, CanNotify ? "notify" : null, CanIndicate ? "indicate" : null
    }.OfType<string>().ToArray();
}
