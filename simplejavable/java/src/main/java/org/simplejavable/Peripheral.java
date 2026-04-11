package org.simplejavable;

import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;


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

    public void setEventListener(EventListener listener) {
        this.eventListener = listener;
    }

    public String getIdentifier() {
        return nativePeripheralIdentifier(adapterId, instanceId) != null ?
               nativePeripheralIdentifier(adapterId, instanceId) : "";
    }

    public BluetoothAddress getAddress() {
        return new BluetoothAddress(nativePeripheralAddress(adapterId, instanceId) != null ?
                                  nativePeripheralAddress(adapterId, instanceId) : "");
    }

    public BluetoothAddressType getAddressType() {
        return BluetoothAddressType.fromInt(nativePeripheralAddressType(adapterId, instanceId));
    }

    public int getRssi() {
        return nativePeripheralRssi(adapterId, instanceId);
    }

    public int getTxPower() {
        return nativePeripheralTxPower(adapterId, instanceId);
    }

    public int getMtu() {
        return nativePeripheralMtu(adapterId, instanceId);
    }

    public void connect() {
        nativePeripheralConnect(adapterId, instanceId);
    }

    public CompletableFuture<Void> connectAsync() {
        return CompletableFuture.runAsync(this::connect);
    }

    public void disconnect() {
        nativePeripheralDisconnect(adapterId, instanceId);
    }

    public CompletableFuture<Void> disconnectAsync() {
        return CompletableFuture.runAsync(this::disconnect);
    }

    public boolean isConnected() {
        return nativePeripheralIsConnected(adapterId, instanceId);
    }

    public boolean isConnectable() {
        return nativePeripheralIsConnectable(adapterId, instanceId);
    }

    public boolean isPaired() {
        return nativePeripheralIsPaired(adapterId, instanceId);
    }

    public void unpair() {
        nativePeripheralUnpair(adapterId, instanceId);
    }

    public List<Service> services() {
        return nativePeripheralServices(adapterId, instanceId);
    }

    public Map<Integer, byte[]> manufacturerData() {
        return nativePeripheralManufacturerData(adapterId, instanceId);
    }

    public byte[] read(BluetoothUUID service, BluetoothUUID characteristic) {
        return nativePeripheralRead(adapterId, instanceId, service.toString(), characteristic.toString());
    }

    public CompletableFuture<byte[]> readAsync(BluetoothUUID service, BluetoothUUID characteristic) {
        return CompletableFuture.supplyAsync(() -> read(service, characteristic));
    }

    public void writeRequest(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        nativePeripheralWriteRequest(adapterId, instanceId, service.toString(), characteristic.toString(), data);
    }

    public CompletableFuture<Void> writeRequestAsync(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        return CompletableFuture.runAsync(() -> writeRequest(service, characteristic, data));
    }

    public void writeCommand(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        nativePeripheralWriteCommand(adapterId, instanceId, service.toString(), characteristic.toString(), data);
    }

    public CompletableFuture<Void> writeCommandAsync(BluetoothUUID service, BluetoothUUID characteristic, byte[] data) {
        return CompletableFuture.runAsync(() -> writeCommand(service, characteristic, data));
    }

    public void notify(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        nativePeripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), callback);
    }

    public CompletableFuture<Void> notifyAsync(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        return CompletableFuture.runAsync(() -> notify(service, characteristic, callback));
    }

    public void indicate(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        nativePeripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), callback);
    }

    public CompletableFuture<Void> indicateAsync(BluetoothUUID service, BluetoothUUID characteristic, DataCallback callback) {
        return CompletableFuture.runAsync(() -> indicate(service, characteristic, callback));
    }

    public void unsubscribe(BluetoothUUID service, BluetoothUUID characteristic) {
        nativePeripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString());
    }

    public CompletableFuture<Void> unsubscribeAsync(BluetoothUUID service, BluetoothUUID characteristic) {
        return CompletableFuture.runAsync(() -> unsubscribe(service, characteristic));
    }

    public byte[] read(BluetoothUUID service, BluetoothUUID characteristic, BluetoothUUID descriptor) {
        return nativePeripheralDescriptorRead(adapterId, instanceId, service.toString(), characteristic.toString(), descriptor.toString());
    }

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