namespace SimpleSharpBLE;

/// <summary>A managed snapshot of a remote service; GATT operations belong to Peripheral.</summary>
public sealed record Service(string Uuid, byte[] Data, IReadOnlyList<Characteristic> Characteristics);
