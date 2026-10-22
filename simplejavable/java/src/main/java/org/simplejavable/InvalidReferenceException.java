package org.simplejavable;

/**
 * An internal object reference cannot be resolved.
 */
public class InvalidReferenceException extends SimpleJavaBleException {
    public InvalidReferenceException(String message) {
        super(ErrorCode.INVALID_BACKEND_REFERENCE, message);
    }
}
