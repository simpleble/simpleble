package org.simplejavable;

/**
 * Represents a Bluetooth MAC address.
 */
public class BluetoothAddress {
    private final String address;

    public BluetoothAddress(String address) {
        this.address = address;
    }

    /**
     * Returns the string representation of the Bluetooth address.
     * @return The string address.
     */
    @Override
    public String toString() {
        return address;
    }
}