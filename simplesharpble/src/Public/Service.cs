namespace SimpleSharpBLE;

/// <summary>A managed snapshot of a remote service; GATT operations belong to Peripheral.</summary>
/// <param name="Uuid">The service UUID.</param>
/// <param name="Data">Advertised service data, if any.</param>
/// <param name="Characteristics">Discovered characteristics; empty for advertised-only services.</param>
public sealed record Service(string Uuid, byte[] Data, IReadOnlyList<Characteristic> Characteristics);
