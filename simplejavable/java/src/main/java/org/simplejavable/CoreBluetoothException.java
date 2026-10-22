package org.simplejavable;

/**
 * CoreBluetooth reported a failure.
 */
public class CoreBluetoothException extends SimpleJavaBleException {
    public CoreBluetoothException(String message) {
        super(ErrorCode.CORE_BLUETOOTH_EXCEPTION, message);
    }
}
