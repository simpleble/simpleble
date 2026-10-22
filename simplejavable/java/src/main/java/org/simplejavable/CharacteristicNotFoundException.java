package org.simplejavable;

/**
 * The requested characteristic UUID is absent from the selected GATT service.
 */
public class CharacteristicNotFoundException extends SimpleJavaBleException {
    public CharacteristicNotFoundException(String message) {
        super(ErrorCode.GATT_CHARACTERISTIC_NOT_FOUND, message);
    }
}
