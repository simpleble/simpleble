package org.simplejavable;

/**
 * The requested service UUID is absent from the discovered GATT services.
 */
public class ServiceNotFoundException extends SimpleJavaBleException {
    public ServiceNotFoundException(String message) {
        super(ErrorCode.GATT_SERVICE_NOT_FOUND, message);
    }
}
