package org.simplejavable;

import java.util.List;

/**
 * Represents a Bluetooth Low Energy (BLE) service.
 */
public class Service {
    private final String uuid;
    private final List<Characteristic> characteristics;

    public Service(String uuid, List<Characteristic> characteristics) {
        this.uuid = uuid;
        this.characteristics = characteristics;
    }

    /**
     * Gets the UUID of the service.
     * @return The service UUID.
     */
    public String uuid() {
        return uuid;
    }

    /**
     * Gets the characteristics associated with this service.
     * @return A list of characteristics.
     */
    public List<Characteristic> characteristics() {
        return characteristics;
    }
}