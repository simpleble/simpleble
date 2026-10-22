package org.simplejavable;

/**
 * The adapter, the characteristic, or this platform does not support the operation.
 */
public class OperationNotSupportedException extends SimpleJavaBleException {
    public OperationNotSupportedException(String message) {
        super(ErrorCode.OPERATION_NOT_SUPPORTED, message);
    }
}
