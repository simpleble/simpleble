package org.simplejavable;

import java.util.Collections;
import java.util.EnumSet;
import java.util.Objects;
import java.util.Set;
import java.util.function.Supplier;

/**
 * A characteristic of a {@link LocalService}.
 * <p>
 * Clients read the stored value unless a read handler supplies one. Writes from clients update the stored value
 * before {@link EventListener#onWrite(byte[])} runs.
 */
public class LocalCharacteristic {
    private final long instanceId;
    private final BluetoothUUID uuid;
    private final Set<LocalCharacteristicCapability> capabilities;
    private volatile Supplier<byte[]> readHandler;
    private volatile EventListener eventListener;

    private final Callback callbacks = new Callback() {
        @Override
        public byte[] onRead() {
            Supplier<byte[]> handler = readHandler;
            return handler != null ? handler.get() : null;
        }

        @Override
        public void onWrite(byte[] value) {
            EventListener listener = eventListener;
            if (listener != null) {
                listener.onWrite(value);
            }
        }

        @Override
        public void onSubscribed() {
            EventListener listener = eventListener;
            if (listener != null) {
                listener.onSubscribed();
            }
        }

        @Override
        public void onUnsubscribed() {
            EventListener listener = eventListener;
            if (listener != null) {
                listener.onUnsubscribed();
            }
        }
    };

    LocalCharacteristic(long newInstanceId) {
        this.instanceId = newInstanceId;
        this.uuid = new BluetoothUUID(nativeUuid(instanceId));

        int mask = nativeCapabilities(instanceId);
        EnumSet<LocalCharacteristicCapability> values = EnumSet.noneOf(LocalCharacteristicCapability.class);
        for (LocalCharacteristicCapability capability : LocalCharacteristicCapability.values()) {
            if ((mask & capability.getBit()) != 0) {
                values.add(capability);
            }
        }
        this.capabilities = Collections.unmodifiableSet(values);

        nativeRegister(instanceId, callbacks);
    }

    /**
     * Gets the UUID of the characteristic.
     * @return The characteristic UUID.
     */
    public BluetoothUUID getUuid() {
        return uuid;
    }

    /**
     * Gets the operations offered to clients.
     * @return An unmodifiable set of capabilities.
     */
    public Set<LocalCharacteristicCapability> getCapabilities() {
        return capabilities;
    }

    /**
     * Gets a copy of the stored value.
     * @return The stored value.
     */
    public byte[] getValue() {
        return nativeValue(instanceId);
    }

    /**
     * Stores a value. With {@link LocalCharacteristicCapability#NOTIFY} or
     * {@link LocalCharacteristicCapability#INDICATE}, it is also published to subscribed clients on a best-effort
     * basis. This can be called from {@link EventListener#onWrite(byte[])} to echo a write.
     * @param value The value to store.
     */
    public void setValue(byte[] value) {
        nativeSetValue(instanceId, Objects.requireNonNull(value, "value"));
    }

    /**
     * Sets a handler that supplies the value of each client read.
     * <p>
     * NOTE: The handler runs on a native thread while the client waits, so it must return quickly. If it returns
     * null or throws, the client reads the stored value.
     * @param handler The read handler, or null to serve the stored value.
     */
    public void setReadHandler(Supplier<byte[]> handler) {
        this.readHandler = handler;
        nativeSetReadHandler(instanceId, handler != null);
    }

    /**
     * Sets the listener for write and subscription events.
     * <p>
     * Events run on SimpleJavaBLE's callback thread and must return promptly.
     * @param listener The EventListener implementation, or null to remove it.
     */
    public void setEventListener(EventListener listener) {
        this.eventListener = listener;
    }

    private native void nativeRegister(long characteristicId, Callback callback);
    private native String nativeUuid(long characteristicId);
    private native int nativeCapabilities(long characteristicId);
    private native byte[] nativeValue(long characteristicId);
    private native void nativeSetValue(long characteristicId, byte[] value);
    private native void nativeSetReadHandler(long characteristicId, boolean enabled);

    private interface Callback {
        byte[] onRead();
        void onWrite(byte[] value);
        void onSubscribed();
        void onUnsubscribed();
    }

    /**
     * Receives write and subscription events of a LocalCharacteristic.
     */
    public interface EventListener {
        /**
         * Called after a client writes; the stored value is already updated.
         * @param value A copy of the written bytes.
         */
        default void onWrite(byte[] value) {}

        /**
         * Called when the first client subscribes.
         */
        default void onSubscribed() {}

        /**
         * Called when the last client unsubscribes.
         */
        default void onUnsubscribed() {}
    }

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
