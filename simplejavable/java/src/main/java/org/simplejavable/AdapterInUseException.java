package org.simplejavable;

/**
 * Another process holds the adapter, such as a Dongl whose serial port another program has open.
 */
public class AdapterInUseException extends SimpleJavaBleException {
    public AdapterInUseException(String message) {
        super(ErrorCode.ADAPTER_IN_USE, message);
    }
}
