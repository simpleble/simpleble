package org.simplejavable;

/**
 * An operation failed without a more specific classification. The message has any available detail.
 */
public class OperationFailedException extends SimpleJavaBleException {
    public OperationFailedException(String message) {
        super(ErrorCode.OPERATION_FAILED, message);
    }
}
