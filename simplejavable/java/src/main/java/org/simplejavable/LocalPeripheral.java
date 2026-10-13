package org.simplejavable;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * A BLE peripheral hosted by this process, which advertises and serves local GATT services.
 * <p>
 * {@link Peripheral} is a remote device. A LocalPeripheral is this host. Create one with
 * {@link Adapter#createLocalPeripheral()}, configure its services and advertised service UUIDs, then call
 * {@link #start()}. The configuration cannot change while started.
 */
public class LocalPeripheral {
    private final long instanceId;
    private final List<LocalService> services = new ArrayList<>();
    private volatile EventListener eventListener;

    private final Callback callbacks = new Callback() {
        @Override
        public void onClientConnected(String address) {
            EventListener listener = eventListener;
            if (listener != null) {
                listener.onClientConnected(new BluetoothAddress(address));
            }
        }

        @Override
        public void onClientDisconnected(String address) {
            EventListener listener = eventListener;
            if (listener != null) {
                listener.onClientDisconnected(new BluetoothAddress(address));
            }
        }
    };

    LocalPeripheral(long newInstanceId) {
        this.instanceId = newInstanceId;
        nativeRegister(instanceId, callbacks);
    }

    long getInstanceId() {
        return instanceId;
    }

    /**
     * Sets the listener for client connection events.
     * <p>
     * Events run on a native thread.
     * <p>
     * NOTE: CoreBluetooth doesn't report connections on macOS, so these events follow a client's first
     * subscription and last unsubscription instead.
     * @param listener The EventListener implementation, or null to remove it.
     */
    public void setEventListener(EventListener listener) {
        this.eventListener = listener;
    }

    /**
     * Adds a service UUID to the advertisement. Call before {@link #start()}.
     * @param serviceUuid The UUID to advertise.
     */
    public void addAdvertisedService(BluetoothUUID serviceUuid) {
        addAdvertisedService(List.of(serviceUuid));
    }

    /**
     * Adds service UUIDs to the advertisement. Call before {@link #start()}.
     * @param serviceUuids The UUIDs to advertise.
     */
    public void addAdvertisedService(List<BluetoothUUID> serviceUuids) {
        String[] uuids = serviceUuids.stream()
            .map(uuid -> Objects.requireNonNull(uuid, "serviceUuids contains null").toString())
            .toArray(String[]::new);
        nativeAddAdvertisedService(instanceId, uuids);
    }

    /**
     * Adds a primary GATT service. Call before {@link #start()}.
     * @param uuid The service UUID.
     * @return The new LocalService.
     */
    public LocalService addService(BluetoothUUID uuid) {
        Objects.requireNonNull(uuid, "uuid");
        synchronized (services) {
            LocalService service = new LocalService(nativeAddService(instanceId, uuid.toString()));
            services.add(service);
            return service;
        }
    }

    /**
     * Gets the services added to this peripheral.
     * @return A list of LocalService objects.
     */
    public List<LocalService> getServices() {
        synchronized (services) {
            return new ArrayList<>(services);
        }
    }

    /**
     * Removes every service so the GATT layout can be rebuilt. Call while stopped.
     */
    public void removeAllServices() {
        synchronized (services) {
            nativeRemoveAllServices(instanceId);
            services.clear();
        }
    }

    /**
     * Publishes the services and starts advertising.
     * <p>
     * Keep the process running while started.
     */
    public void start() {
        nativeStart(instanceId);
    }

    /**
     * Stops advertising and hosting the services.
     */
    public void stop() {
        nativeStop(instanceId);
    }

    /**
     * Checks if the services are published.
     * @return True if started.
     */
    public boolean isStarted() {
        return nativeIsStarted(instanceId);
    }

    /**
     * Checks if the peripheral is advertising.
     * @return True if advertising.
     */
    public boolean isAdvertising() {
        return nativeIsAdvertising(instanceId);
    }

    private native void nativeRegister(long peripheralId, Callback callback);
    private native void nativeAddAdvertisedService(long peripheralId, String[] serviceUuids);
    private native long nativeAddService(long peripheralId, String uuid);
    private native void nativeRemoveAllServices(long peripheralId);
    private native void nativeStart(long peripheralId);
    private native void nativeStop(long peripheralId);
    private native boolean nativeIsStarted(long peripheralId);
    private native boolean nativeIsAdvertising(long peripheralId);

    private interface Callback {
        void onClientConnected(String address);
        void onClientDisconnected(String address);
    }

    /**
     * Receives client connection events of a LocalPeripheral.
     */
    public interface EventListener {
        /**
         * Called when a client connects.
         * @param address The client's address, or a platform identifier where the address is not exposed.
         */
        default void onClientConnected(BluetoothAddress address) {}

        /**
         * Called when a client disconnects.
         * @param address The client's address, or a platform identifier where the address is not exposed.
         */
        default void onClientDisconnected(BluetoothAddress address) {}
    }

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
