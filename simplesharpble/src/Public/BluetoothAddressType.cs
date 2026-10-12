namespace SimpleSharpBLE;

/// <summary>The type of a Bluetooth device address.</summary>
public enum BluetoothAddressType
{
    /// <summary>A public, IEEE-assigned address.</summary>
    Public = 0,
    /// <summary>A random static or private address.</summary>
    Random = 1,
    /// <summary>The platform did not report the type.</summary>
    Unspecified = 2,
}
