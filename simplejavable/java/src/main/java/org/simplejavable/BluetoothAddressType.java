package org.simplejavable;

/**
 * Represents the type of a Bluetooth MAC address.
 */
public enum BluetoothAddressType {
    PUBLIC(0),
    RANDOM(1),
    UNSPECIFIED(2);

    private final int value;

    BluetoothAddressType(int value) {
        this.value = value;
    }

    /**
     * Gets the integer value of the address type mapping.
     * @return The integer value of the address type.
     */
    public int getValue() {
        return value;
    }

    @Override
    public String toString() {
        switch (this) {
            case PUBLIC:
                return "Public";
            case RANDOM:
                return "Random";
            case UNSPECIFIED:
                return "Unspecified";
            default:
                return "Unspecified";
        }
    }

    /**
     * Parses a BluetoothAddressType from its integer representation.
     * @param value The integer value.
     * @return The corresponding BluetoothAddressType.
     */
    public static BluetoothAddressType fromInt(int value) {
        for (BluetoothAddressType type : values()) {
            if (type.getValue() == value) {
                return type;
            }
        }
        return UNSPECIFIED;
    }
}