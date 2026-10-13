package org.simplejavable;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.Set;

/**
 * A primary GATT service hosted by a {@link LocalPeripheral}.
 */
public class LocalService {
    private final long instanceId;
    private final BluetoothUUID uuid;
    private final List<LocalCharacteristic> characteristics = new ArrayList<>();

    LocalService(long newInstanceId) {
        this.instanceId = newInstanceId;
        this.uuid = new BluetoothUUID(nativeUuid(instanceId));
    }

    /**
     * Gets the UUID of the service.
     * @return The service UUID.
     */
    public BluetoothUUID getUuid() {
        return uuid;
    }

    /**
     * Gets the characteristics added to this service.
     * @return A list of LocalCharacteristic objects.
     */
    public List<LocalCharacteristic> getCharacteristics() {
        synchronized (characteristics) {
            return new ArrayList<>(characteristics);
        }
    }

    /**
     * Adds a characteristic. Call before {@link LocalPeripheral#start()}.
     * @param uuid The characteristic UUID.
     * @param capabilities The operations offered to clients; at least one is required.
     * @return The new LocalCharacteristic.
     */
    public LocalCharacteristic addCharacteristic(BluetoothUUID uuid, LocalCharacteristicCapability... capabilities) {
        return addCharacteristic(uuid, Set.copyOf(Arrays.asList(capabilities)));
    }

    /**
     * Adds a characteristic. Call before {@link LocalPeripheral#start()}.
     * @param uuid The characteristic UUID.
     * @param capabilities The operations offered to clients; at least one is required.
     * @return The new LocalCharacteristic.
     */
    public LocalCharacteristic addCharacteristic(BluetoothUUID uuid, Set<LocalCharacteristicCapability> capabilities) {
        Objects.requireNonNull(uuid, "uuid");
        if (capabilities.isEmpty()) {
            throw new IllegalArgumentException("A local characteristic requires at least one capability.");
        }

        int mask = 0;
        for (LocalCharacteristicCapability capability : capabilities) {
            mask |= capability.getBit();
        }

        synchronized (characteristics) {
            LocalCharacteristic characteristic = new LocalCharacteristic(
                nativeAddCharacteristic(instanceId, uuid.toString(), mask));
            characteristics.add(characteristic);
            return characteristic;
        }
    }

    private native String nativeUuid(long serviceId);
    private native long nativeAddCharacteristic(long serviceId, String uuid, int capabilities);

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
