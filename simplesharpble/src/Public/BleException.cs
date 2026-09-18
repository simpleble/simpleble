namespace SimpleSharpBLE;

/// <summary>The failure categories reported by SimpleBLE's C ABI.</summary>
public enum BleErrorCode
{
    InvalidArgument = 0,
    OutOfMemory = 1,
    ObjectNotInitialized = 2,
    InvalidBackendReference = 3,
    PeripheralNotConnected = 4,
    GattServiceNotFound = 5,
    GattCharacteristicNotFound = 6,
    GattDescriptorNotFound = 7,
    OperationNotSupported = 8,
    OperationFailed = 9,
    WinRtAccessDenied = 10,
    WinRtException = 11,
    CoreBluetoothException = 12,
    UnclassifiedException = 13,
}

/// <summary>A native BLE operation failed. Code preserves the native failure category.</summary>
public sealed class BleException : Exception
{
    public BleErrorCode Code { get; }

    internal BleException(BleErrorCode code, string message) : base(message) => Code = code;
}
