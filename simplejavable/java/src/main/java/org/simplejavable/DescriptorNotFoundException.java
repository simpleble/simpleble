package org.simplejavable;

/**
 * The requested descriptor UUID is absent from the selected GATT characteristic.
 */
public class DescriptorNotFoundException extends SimpleJavaBleException {
    public DescriptorNotFoundException(String message) {
        super(ErrorCode.GATT_DESCRIPTOR_NOT_FOUND, message);
    }
}
