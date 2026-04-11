package org.simplejavable;

import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;


/**
 * Represents a remote Bluetooth Low Energy peripheral device.
 */
public class Peripheral {
    private final long instanceId;
    private final long adapterId;
    private EventListener eventListener;

    Peripheral(long newAdapterId, long newInstanceId) {
        this.instanceId = newInstanceId;
        this.adapterId = newAdapterId;
        nativePeripheralRegister(adapterId, instanceId, callbacks);
    }

    private final Callback callbacks = new Callback() {
        @Override
        public void onConnected() {
            if (eventListener != null) {
                eventListener.onConnected();
            }
        }

        @Override
        public void onDisconnected() {
            if (eventListener != null) {
                eventListener.onDisconnected();
            }
        }
    };

    /**
     * Sets the event listener for peripheral events (e.g., connected, disconnected).
     * @param listener The EventListener implementation.
     */
    public void setEventListener(EventListener listener) {
        this.eventListener = listener;
    }

    /**
     * Gets the identifier (name) of the peripheral.
     * @return The string identifier.
     */
    public String getIdentifier() {
        return nativePeripheralIdentifier(adapterId, instanceId) != null ?
               nativePeripheralIdentifier(adapterId, instanceId) : "";
    }

    /**
     * Gets the primary MAC address of the peripheral.
     * @return The BluetoothAddress.
     */
    public BluetoothAddress getAddress() {
        return new BluetoothAddress(nativePeripheralAddress(adapterId, instanceId) != null ?
                                  nativePeripheralAddress(adapterId, instanceId) : "");
    }

    /**
     * Gets the address type (public, random, etc) of the peripheral.
     * @return The BluetoothAddressType.
     */
    public BluetoothAddressType getAddressType() {
        return BluetoothAddressType.fromInt(nativePeripheralAddressType(adapterId, instanceId));
    }

    /**
     * Gets the last recorded RSSI (Received Signal Strength Indicator) of the peripheral.
     * @return The RSSI value in dBm.
     */
    public int getRssi() {
        return nativePeripheralRssi(adapterId, instanceId);
    }

    /**
     * Gets the TX power of the peripheral.
     * @return The TX power value in dBm.
     */
    public int getTxPower() {
        return nativePeripheralTxPower(adapterId, instanceId);
    }

    /**
     * Gets the Maximum Transmission Unit (MTU) size for the connection.
     * @return The MTU size in bytes.
     */
    public int getMtu() {
        return nativePeripheralMtu(adapterId, instanceId);
    }

    /**
     * Connects to the peripheral synchronously.
     */
    public void connect() {
        nativePeripheralConnect(adapterId, instanceId);
    }

    /**
     * Connects to the peripheral asynchronously.
     * @return A CompletableFuture that completes when connected.
     */
    public CompletableFuture<Void> connectAsync() {
        return CompletableFuture.runAsync(this::connect);
    }

    /**
     * Disconnects from the peripheral synchronously.
     */
    public void disconnect() {
        nativePeripheralDisconnect(adapterId, instanceId);
    }

    /**
     * Disconnects from the peripheral asynchronously.
     * @return A CompletableFuture that completes when disconnected.
     */
    public CompletableFuture<Void> disconnectAsync() {
        return CompletableFuture.runAsync(this::disconnect);
    }

    /**
     * Checks if the peripheral is currently connected.
     * @return True if connected.
     */
    public boolean isConnected() {
        return nativePeripheralIsConnected(adapterId, instanceId);
    }

    /**
     * Checks if the peripheral is connectable.
     * @return True if connectable.
     */
    public boolean isConnectable() {
        return nativePeripheralIsConnectable(adapterId, instanceId);
    }

    /**
     * Checks if the peripheral is paired with the system.
     * @return True if paired.
     */
    public boolean isPaired() {
        return nativePeripheralIsPaired(adapterId, instanceId);
    }

    /**
     * Unpairs the peripheral from the host system.
     */
    public void unpair() {
        nativePeripheralUnpair(adapterId, instanceId);
    }

    /**
     * Retrieves the list of services offered by the peripheral.
     * @return A list of Service objects.
     */
    public List<Service> services() {
        return nativePeripheralServices(adapterId, instanceId);
    }

    /**
     * Retrieves the manufacturer-specific data advertised by the peripheral.
     * @return A map of manufacturer IDs to byte arrays.
     */
    public Map<Integer, byte[]> manufacturerData() {
        return nativePeripheralManufacturerData(adapterId, instanceId);
    }

    /**
     * Reads the value of a characteristic synchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @return The read byte array payload.
     */
    public byte[] read(BluetoothUUID service, BluetoothUUID characteristic) {
        return nativePeripheralRead(adapterId, instanceId, service.toString(), characteristic.toString());
    }

    /**
     * Reads the value of a characteristic asynchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @return A CompletableFuture containing the byte array payload.
     */
    public CompletableFuture<byte[]> readAsync(BluetoothUUID service, BluetoothUUID characteristic) {
        return CompletableFuture.supplyAsync(() -> read(service, characteristic));
    }

    /**
     * Writes to a characteristic synchronously, expecting a response.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param data The payload to write.
     */
    public void writeRequest(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        nativePeripheralWriteRequest(adapterId, instanceId, service.toString(), characteristic.toString(), data);
    }

    /**
     * Writes to a characteristic asynchronously, expecting a response.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param data The payload to write.
     * @return A CompletableFuture that completes when written.
     */
    public CompletableFuture<Void> writeRequestAsync(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        return CompletableFuture.runAsync(() -> writeRequest(service, characteristic, data));
    }

    /**
     * Writes to a characteristic synchronously without expecting a response.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param data The payload to write.
     */
    public void writeCommand(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        nativePeripheralWriteCommand(adapterId, instanceId, service.toString(), characteristic.toString(), data);
    }

    /**
     * Writes to a characteristic asynchronously without expecting a response.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param data The payload to write.
     * @return A CompletableFuture that completes when written.
     */
    public CompletableFuture<Void> writeCommandAsync(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        return CompletableFuture.runAsync(() -> writeCommand(service, characteristic, data));
    }

    /**
     * Subscribes to notifications from a characteristic synchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param callback The callback invoked on data reception.
     */
    public void notify(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        nativePeripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), callback);
    }

    /**
     * Subscribes to notifications from a characteristic asynchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param callback The callback invoked on data reception.
     * @return A CompletableFuture that completes when subscribed.
     */
    public CompletableFuture<Void> notifyAsync(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        return CompletableFuture.runAsync(() -> notify(service, characteristic, callback));
    }

    /**
     * Subscribes to indications from a characteristic synchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param callback The callback invoked on data reception.
     */
    public void indicate(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        nativePeripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), callback);
    }

    /**
     * Subscribes to indications from a characteristic asynchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @param callback The callback invoked on data reception.
     * @return A CompletableFuture that completes when subscribed.
     */
    public CompletableFuture<Void> indicateAsync(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        return CompletableFuture.runAsync(() -> indicate(service, characteristic, callback));
    }

    /**
     * Unsubscribes from notifications or indications synchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     */
    public void unsubscribe(BluetoothUUID service, BluetoothUUID characteristic) {
        nativePeripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString());
    }

    /**
     * Unsubscribes from notifications or indications asynchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @return A CompletableFuture that completes when unsubscribed.
     */
    public CompletableFuture<Void> unsubscribeAsync(BluetoothUUID service, BluetoothUUID characteristic) {
        return CompletableFuture.runAsync(() -> unsubscribe(service, characteristic));
    }

    /**
     * Reads the value of a characteristic synchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @return The read byte array payload.
     */
    public byte[] read(BluetoothUUID service, BluetoothUUID characteristic, BluetoothUUID descriptor) {
        return nativePeripheralDescriptorRead(adapterId, instanceId, service.toString(), characteristic.toString(), descriptor.toString());
    }

    /**
     * Reads the value of a characteristic asynchronously.
     * @param service The UUID of the service.
     * @param characteristic The UUID of the characteristic.
     * @return A CompletableFuture containing the byte array payload.
     */
    public CompletableFuture<byte[]> readAsync(BluetoothUUID service, BluetoothUUID characteristic, BluetoothUUID descriptor) {
        return CompletableFuture.supplyAsync(() -> read(service, characteristic, descriptor));
    }

    public void write(BluetoothUUID service, BluetoothUUID characteristic, BluetoothUUID descriptor, byte[] data) {
        nativePeripheralDescriptorWrite(adapterId, instanceId, service.toString(), characteristic.toString(), descriptor.toString(), data);
    }

    public CompletableFuture<Void> writeAsync(BluetoothUUID service, BluetoothUUID characteristic, BluetoothUUID descriptor, byte[] data) {
        return CompletableFuture.runAsync(() -> write(service, characteristic, descriptor, data));
    }

    private native void nativePeripheralRegister(long adapterId, long instanceId, Callback callback);
    private native String nativePeripheralIdentifier(long adapterId, long instanceId);
    private native String nativePeripheralAddress(long adapterId, long instanceId);
    private native int nativePeripheralAddressType(long adapterId, long instanceId);
    private native int nativePeripheralRssi(long adapterId, long instanceId);
    private native int nativePeripheralTxPower(long adapterId, long instanceId);
    private native int nativePeripheralMtu(long adapterId, long instanceId);
    private native void nativePeripheralConnect(long adapterId, long instanceId);
    private native void nativePeripheralDisconnect(long adapterId, long instanceId);
    private native boolean nativePeripheralIsConnected(long adapterId, long instanceId);
    private native boolean nativePeripheralIsConnectable(long adapterId, long instanceId);
    private native boolean nativePeripheralIsPaired(long adapterId, long instanceId);
    private native void nativePeripheralUnpair(long adapterId, long instanceId);
    private native List<Service> nativePeripheralServices(long adapterId, long instanceId);
    private native Map<Integer, byte[]> nativePeripheralManufacturerData(long adapterId, long instanceId);
    private native byte[] nativePeripheralRead(long adapterId, long instanceId, String service, String characteristic);
    private native void nativePeripheralWriteRequest(long adapterId, long instanceId, String service, String characteristic, byte[] data);
    private native void nativePeripheralWriteCommand(long adapterId, long instanceId, String service, String characteristic, byte[] data);
    private native void nativePeripheralNotify(long adapterId, long instanceId, String service, String characteristic, DataCallback callback);
    private native void nativePeripheralIndicate(long adapterId, long instanceId, String service, String characteristic, DataCallback callback);
    private native void nativePeripheralUnsubscribe(long adapterId, long instanceId, String service, String characteristic);
    private native byte[] nativePeripheralDescriptorRead(long adapterId, long instanceId, String service, String characteristic, String descriptor);
    private native void nativePeripheralDescriptorWrite(long adapterId, long instanceId, String service, String characteristic, String descriptor, byte[] data);

    public interface DataCallback {
        void onDataReceived(byte[] data);
    }

    private interface Callback {
        void onConnected();
        void onDisconnected();
    }

    public interface EventListener {
        default void onConnected() {}
        default void onDisconnected() {}
    }
}