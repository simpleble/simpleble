package org.simplejavable;

/**
 * SimpleBLE rejected an argument.
 */
public class InvalidArgumentException extends SimpleJavaBleException {
    public InvalidArgumentException(String message) {
        super(ErrorCode.INVALID_ARGUMENT, message);
    }
}
