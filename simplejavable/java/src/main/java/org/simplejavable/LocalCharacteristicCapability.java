package org.simplejavable;

/**
 * Operations that a local characteristic offers to clients.
 */
public enum LocalCharacteristicCapability {
    /** Clients can read the value. */
    READ(1),
    /** Clients can write with acknowledgement. */
    WRITE_REQUEST(2),
    /** Clients can write without acknowledgement. */
    WRITE_COMMAND(4),
    /** Clients can subscribe to notifications. */
    NOTIFY(8),
    /** Clients can subscribe to indications. */
    INDICATE(16);

    private final int bit;

    LocalCharacteristicCapability(int bit) {
        this.bit = bit;
    }

    int getBit() {
        return bit;
    }
}
