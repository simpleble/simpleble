package org.simplejavable;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Represents a physical Bluetooth adapter on the host system.
 */
public class Adapter {
    // One wrapper per native adapter, so listeners and peripheral wrappers survive later enumerations.
    private static final Map<Long, Adapter> adapterCache = new ConcurrentHashMap<>();

    private volatile EventListener eventListener;
    private final long instanceId;
    private final Map<Long, Peripheral> peripheralCache = new ConcurrentHashMap<>();

    private final Callback callbacks = new Callback() {
        @Override
        public void onPowerOn() {
            if (eventListener != null) {
                eventListener.onPowerOn();
            }
        }

        @Override
        public void onPowerOff() {
            if (eventListener != null) {
                eventListener.onPowerOff();
            }
        }

        @Override
        public void onScanStart() {
            if (eventListener != null) {
                eventListener.onScanStart();
            }
        }

        @Override
        public void onScanStop() {
            if (eventListener != null) {
                eventListener.onScanStop();
            }
        }

        @Override
        public void onScanUpdated(long peripheralId) {
            if (eventListener != null) {
                eventListener.onScanUpdated(getPeripheral(peripheralId));
            }
        }

        @Override
        public void onScanFound(long peripheralId) {
            if (eventListener != null) {
                eventListener.onScanFound(getPeripheral(peripheralId));
            }
        }
    };

    private Adapter(long newInstanceId) {
        this.instanceId = newInstanceId;
        nativeAdapterRegister(instanceId, callbacks);
    }

    /**
     * Gets the system identifier of the Bluetooth adapter.
     * @return The string identifier.
     */
    public String getIdentifier() {
        String identifier = nativeAdapterIdentifier(instanceId);
        return identifier != null ? identifier : "";
    }

    /**
     * Gets the MAC address of the Bluetooth adapter.
     * @return The BluetoothAddress.
     */
    public BluetoothAddress getAddress() {
        String address = nativeAdapterAddress(instanceId);
        return new BluetoothAddress(address != null ? address : "");
    }

    /**
     * Turns the Bluetooth adapter on, where the platform allows it.
     */
    public void powerOn() {
        nativeAdapterPowerOn(instanceId);
    }

    /**
     * Turns the Bluetooth adapter off, where the platform allows it.
     */
    public void powerOff() {
        nativeAdapterPowerOff(instanceId);
    }

    /**
     * Checks if the Bluetooth adapter is powered on.
     * @return True if the adapter is powered on.
     */
    public boolean isPowered() {
        return nativeAdapterIsPowered(instanceId);
    }

    /**
     * Starts scanning for BLE peripherals asynchronously.
     */
    public void scanStart() {
        nativeAdapterScanStart(instanceId);
    }

    /**
     * Stops an active BLE peripheral scan.
     */
    public void scanStop() {
        nativeAdapterScanStop(instanceId);
    }

    /**
     * Starts scanning for BLE peripherals, blocking for the specified duration.
     * @param timeoutMs The scan duration in milliseconds.
     * @throws Exception If the scan fails.
     */
    public void scanFor(int timeoutMs) throws Exception {
        nativeAdapterScanFor(instanceId, timeoutMs);
    }

    /**
     * Scans for BLE peripherals for the specified duration asynchronously.
     * @param timeoutMs The scan duration in milliseconds.
     * @return A CompletableFuture that completes when the scan finishes.
     */
    public CompletableFuture<Void> scanForAsync(int timeoutMs) {
        return CompletableFuture.runAsync(() -> {
            try {
                scanFor(timeoutMs);
            } catch (Exception e) {
                throw new CompletionException(e);
            }
        }, AsyncExecutor.INSTANCE);
    }

    /**
     * Checks if the adapter is currently scanning.
     * @return True if a scan is active.
     */
    public boolean getScanIsActive() {
        return nativeAdapterScanIsActive(instanceId);
    }

    /**
     * Gets the list of peripherals found during the last scan.
     * @return A list of Peripheral objects.
     */
    public List<Peripheral> scanGetResults() {
        long[] results = nativeAdapterScanGetResults(instanceId);
        List<Peripheral> peripherals = new ArrayList<>();
        for (long id : results) {
            peripherals.add(getPeripheral(id));
        }
        return peripherals;
    }

    /**
     * Sets the event listener for adapter events (e.g., power on, scan start, stop, found).
     * @param listener The EventListener implementation.
     */
    public void setEventListener(EventListener listener) {
        this.eventListener = listener;
    }

    /**
     * Gets a list of all peripherals currently paired with the system.
     * @return A list of paired Peripheral objects.
     */
    public List<Peripheral> getPairedPeripherals() {
        long[] results = nativeAdapterGetPairedPeripherals(instanceId);
        List<Peripheral> peripherals = new ArrayList<>();
        for (long id : results) {
            peripherals.add(getPeripheral(id));
        }
        return peripherals;
    }

    /**
     * Gets a list of all peripherals currently connected to the system.
     * @return A list of connected Peripheral objects.
     */
    public List<Peripheral> getConnectedPeripherals() {
        long[] results = nativeAdapterGetConnectedPeripherals(instanceId);
        List<Peripheral> peripherals = new ArrayList<>();
        for (long id : results) {
            peripherals.add(getPeripheral(id));
        }
        return peripherals;
    }

    /**
     * Creates a local peripheral that hosts GATT services and advertises through this adapter.
     * <p>
     * Configure its services and advertised service UUIDs, then call {@link LocalPeripheral#start()}.
     * @return A new LocalPeripheral.
     */
    public LocalPeripheral createLocalPeripheral() {
        return new LocalPeripheral(nativeAdapterCreateLocalPeripheral(instanceId));
    }

    Peripheral getPeripheral(long peripheralId) {
        return peripheralCache.computeIfAbsent(peripheralId, id -> new Peripheral(instanceId, id));
    }

    long getInstanceId() {
        return instanceId;
    }

    static Adapter fromId(long adapterId) {
        return adapterCache.computeIfAbsent(adapterId, Adapter::new);
    }

    /**
     * Checks if Bluetooth is enabled on the host system.
     * @return True if Bluetooth is enabled.
     */
    public static boolean isBluetoothEnabled() {
        return nativeIsBluetoothEnabled();
    }

    /**
     * Gets a list of all available Bluetooth adapters from all active backends.
     * <p>
     * Every call returns the same Adapter object for the same native adapter. Set any {@link Config} values
     * before the first call.
     * @return A list of Adapter objects.
     */
    public static List<Adapter> getAdapters() {
        long[] nativeAdapterIds = nativeGetAdapters();
        List<Adapter> adapters = new ArrayList<>();

        for (long nativeAdapterId : nativeAdapterIds) {
            adapters.add(fromId(nativeAdapterId));
        }

        return adapters;
    }

    // Native method declarations
    private static native long[] nativeGetAdapters();
    private static native boolean nativeIsBluetoothEnabled();
    private native void nativeAdapterRegister(long adapterId, Callback callback);
    private native String nativeAdapterIdentifier(long adapterId);
    private native String nativeAdapterAddress(long adapterId);
    private native void nativeAdapterPowerOn(long adapterId);
    private native void nativeAdapterPowerOff(long adapterId);
    private native boolean nativeAdapterIsPowered(long adapterId);
    private native void nativeAdapterScanStart(long adapterId);
    private native void nativeAdapterScanStop(long adapterId);
    private native void nativeAdapterScanFor(long adapterId, int timeout);
    private native boolean nativeAdapterScanIsActive(long adapterId);
    private native long[] nativeAdapterScanGetResults(long adapterId);
    private native long[] nativeAdapterGetPairedPeripherals(long adapterId);
    private native long[] nativeAdapterGetConnectedPeripherals(long adapterId);
    private native long nativeAdapterCreateLocalPeripheral(long adapterId);

    private interface Callback {
        void onPowerOn();
        void onPowerOff();
        void onScanStart();
        void onScanStop();
        void onScanUpdated(long peripheralId);
        void onScanFound(long peripheralId);
    }

    public interface EventListener {
        default void onPowerOn() {}
        default void onPowerOff() {}
        default void onScanStart() {}
        default void onScanStop() {}
        default void onScanUpdated(Peripheral peripheral) {}
        default void onScanFound(Peripheral peripheral) {}
    }

    static {
        NativeLibraryLoader.ensureLoaded();

        // Runtime.getRuntime().addShutdownHook(new Thread(() -> {
        //     // TODO: We might need this to clean up the native library.
        //     // System.out.println("JVM shutdown initiated at " + System.currentTimeMillis());
        // }));
    }
}
