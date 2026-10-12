namespace SimpleSharpBLE;

/// <summary>The failure categories reported by SimpleBLE's C ABI.</summary>
public enum BleErrorCode
{
    /// <summary>An argument was invalid.</summary>
    InvalidArgument = 0,
    /// <summary>Native memory allocation failed.</summary>
    OutOfMemory = 1,
    /// <summary>The native object was not initialized.</summary>
    ObjectNotInitialized = 2,
    /// <summary>The object's backend is no longer available.</summary>
    InvalidBackendReference = 3,
    /// <summary>The operation requires a connection.</summary>
    PeripheralNotConnected = 4,
    /// <summary>The service UUID was not found on the device.</summary>
    GattServiceNotFound = 5,
    /// <summary>The characteristic UUID was not found in the service.</summary>
    GattCharacteristicNotFound = 6,
    /// <summary>The descriptor UUID was not found on the characteristic.</summary>
    GattDescriptorNotFound = 7,
    /// <summary>The backend or device does not support the operation.</summary>
    OperationNotSupported = 8,
    /// <summary>The platform reported a failure; the message has details.</summary>
    OperationFailed = 9,
    /// <summary>Windows denied Bluetooth access.</summary>
    WinRtAccessDenied = 10,
    /// <summary>A WinRT call failed.</summary>
    WinRtException = 11,
    /// <summary>A CoreBluetooth call failed.</summary>
    CoreBluetoothException = 12,
    /// <summary>Any other native failure.</summary>
    UnclassifiedException = 13,
}

/// <summary>A native BLE operation failed. Code preserves the native failure category.</summary>
public sealed class BleException : Exception
{
    /// <summary>The native failure category.</summary>
    public BleErrorCode Code { get; }

    internal BleException(BleErrorCode code, string message) : base(message) => Code = code;
}
