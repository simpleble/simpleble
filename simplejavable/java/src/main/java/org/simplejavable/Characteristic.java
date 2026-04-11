package org.simplejavable;

import java.util.List;

/**
 * Represents a Bluetooth Low Energy (BLE) characteristic.
 */
public class Characteristic {
    private final String uuid;
    private final List<Descriptor> descriptors;
    private final boolean canRead;
    private final boolean canWriteRequest;
    private final boolean canWriteCommand;
    private final boolean canNotify;
    private final boolean canIndicate;

    public Characteristic(String uuid, List<Descriptor> descriptors, boolean canRead,
                         boolean canWriteRequest, boolean canWriteCommand,
                         boolean canNotify, boolean canIndicate) {
        this.uuid = uuid;
        this.descriptors = descriptors;
        this.canRead = canRead;
        this.canWriteRequest = canWriteRequest;
        this.canWriteCommand = canWriteCommand;
        this.canNotify = canNotify;
        this.canIndicate = canIndicate;
    }

    /**
     * Gets the UUID of the characteristic.
     * @return The characteristic UUID.
     */
    public String uuid() {
        return uuid;
    }

    /**
     * Gets the descriptors associated with this characteristic.
     * @return A list of descriptors.
     */
    public List<Descriptor> descriptors() {
        return descriptors;
    }

    /**
     * Checks if the characteristic supports reading.
     * @return True if read operations are supported.
     */
    public boolean canRead() {
        return canRead;
    }

    /**
     * Checks if the characteristic supports robust write requests.
     * @return True if write requests are supported.
     */
    public boolean canWriteRequest() {
        return canWriteRequest;
    }

    /**
     * Checks if the characteristic supports fast write commands.
     * @return True if write commands are supported.
     */
    public boolean canWriteCommand() {
        return canWriteCommand;
    }

    /**
     * Checks if the characteristic supports notifications.
     * @return True if notifications are supported.
     */
    public boolean canNotify() {
        return canNotify;
    }

    /**
     * Checks if the characteristic supports indications.
     * @return True if indications are supported.
     */
    public boolean canIndicate() {
        return canIndicate;
    }
}