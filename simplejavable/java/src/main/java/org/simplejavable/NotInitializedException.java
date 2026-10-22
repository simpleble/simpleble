package org.simplejavable;

/**
 * A SimpleBLE object has not been initialized.
 */
public class NotInitializedException extends SimpleJavaBleException {
    public NotInitializedException(String message) {
        super(ErrorCode.OBJECT_NOT_INITIALIZED, message);
    }
}
