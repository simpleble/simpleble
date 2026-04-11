package org.simplejavable;

/**
 * Represents a standard Bluetooth UUID used to identify peripheral services or characteristics.
 */
public class BluetoothUUID {
    private final String uuid;

    public BluetoothUUID(String uuid) {
        this.uuid = uuid;
    }

    /**
     * Returns the string representation of the Bluetooth UUID.
     * @return The string UUID.
     */
    @Override
    public String toString() {
        return uuid;
    }
}