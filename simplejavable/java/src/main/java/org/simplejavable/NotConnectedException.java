package org.simplejavable;

/**
 * The operation requires a connected peripheral.
 */
public class NotConnectedException extends SimpleJavaBleException {
    public NotConnectedException(String message) {
        super(ErrorCode.PERIPHERAL_NOT_CONNECTED, message);
    }
}
