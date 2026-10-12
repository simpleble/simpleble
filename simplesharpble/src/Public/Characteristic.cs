namespace SimpleSharpBLE;

/// <summary>A managed snapshot of a remote characteristic's capabilities and descriptors.</summary>
/// <param name="Uuid">The characteristic UUID.</param>
/// <param name="CanRead">Whether the characteristic supports reads.</param>
/// <param name="CanWriteRequest">Whether it supports acknowledged writes.</param>
/// <param name="CanWriteCommand">Whether it supports unacknowledged writes.</param>
/// <param name="CanNotify">Whether it supports notifications.</param>
/// <param name="CanIndicate">Whether it supports indications.</param>
/// <param name="Descriptors">The characteristic's descriptors.</param>
public sealed record Characteristic(string Uuid, bool CanRead, bool CanWriteRequest,
    bool CanWriteCommand, bool CanNotify, bool CanIndicate, IReadOnlyList<Descriptor> Descriptors)
{
    /// <summary>The supported operations as read, write_request, write_command, notify, and indicate.</summary>
    public IReadOnlyList<string> Capabilities => new[]
    {
        CanRead ? "read" : null, CanWriteRequest ? "write_request" : null,
        CanWriteCommand ? "write_command" : null, CanNotify ? "notify" : null, CanIndicate ? "indicate" : null
    }.OfType<string>().ToArray();
}
