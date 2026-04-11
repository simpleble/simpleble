package org.simplejavable;

/**
 * Represents a Bluetooth Low Energy (BLE) descriptor.
 */
public class Descriptor {
    private final String uuid;

    public Descriptor(String uuid) {
        this.uuid = uuid;
    }

    /**
     * Gets the UUID of the descriptor.
     * @return The descriptor UUID.
     */
    public String uuid() {
        return uuid;
    }
}