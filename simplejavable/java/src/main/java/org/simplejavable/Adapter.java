package org.simplejavable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;

/**
 * Represents a physical Bluetooth adapter on the host system.
 */
public class Adapter {
    private EventListener eventListener;
    private final long instanceId;

    private final Callback callbacks = new Callback() {
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
                eventListener.onScanUpdated(new Peripheral(instanceId, peripheralId));
            }
        }

        @Override
        public void onScanFound(long peripheralId) {
            if (eventListener != null) {
                eventListener.onScanFound(new Peripheral(instanceId, peripheralId));
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
     * Starts scanning for BLE peripherals asynchronously and returns the results when finished.
     * @param timeoutMs The scan duration in milliseconds.
     * @return A CompletableFuture containing the list of found peripherals.
     */
    public CompletableFuture<List<Peripheral>> scanForAsync(int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> {
            try {
                scanFor(timeoutMs);
                return scanGetResults();
            } catch (Exception e) {
                throw new CompletionException(e);
            }
        });
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
            peripherals.add(new Peripheral(instanceId, id));
        }
        return peripherals;
    }

    /**
     * Sets the event listener for adapter events (e.g., scan start, stop, found).
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
            peripherals.add(new Peripheral(instanceId, id));
        }
        return peripherals;
    }

    /**
     * Checks if Bluetooth is enabled on the host system.
     * @return True if Bluetooth is enabled.
     */
    public static boolean isBluetoothEnabled() {
        return nativeIsBluetoothEnabled();
    }

    /**
     * Gets a list of all available Bluetooth adapters on the system.
     * @return A list of Adapter objects.
     */
    public static List<Adapter> getAdapters() {
        long[] nativeAdapterIds = nativeGetAdapters();
        List<Adapter> adapters = new ArrayList<>();

        for (long nativeAdapterId : nativeAdapterIds) {
            adapters.add(new Adapter(nativeAdapterId));
        }

        return adapters;
    }

    // Native method declarations
    private static native long[] nativeGetAdapters();
    private static native boolean nativeIsBluetoothEnabled();
    private native void nativeAdapterRegister(long adapterId, Callback callback);
    private native String nativeAdapterIdentifier(long adapterId);
    private native String nativeAdapterAddress(long adapterId);
    private native void nativeAdapterScanStart(long adapterId);
    private native void nativeAdapterScanStop(long adapterId);
    private native void nativeAdapterScanFor(long adapterId, int timeout);
    private native boolean nativeAdapterScanIsActive(long adapterId);
    private native long[] nativeAdapterScanGetResults(long adapterId);
    private native long[] nativeAdapterGetPairedPeripherals(long adapterId);

    private interface Callback {
        void onScanStart();
        void onScanStop();
        void onScanUpdated(long peripheralId);
        void onScanFound(long peripheralId);
    }

    public interface EventListener {
        default void onScanStart() {}
        default void onScanStop() {}
        default void onScanUpdated(Peripheral peripheral) {}
        default void onScanFound(Peripheral peripheral) {}
    }

    static {
        try {
            NativeLibraryLoader.loadLibrary("simplejavable");
        } catch (IOException e) {
            throw new RuntimeException("Failed to load native library", e);
        }

        // Runtime.getRuntime().addShutdownHook(new Thread(() -> {
        //     // TODO: We might need this to clean up the native library.
        //     // System.out.println("JVM shutdown initiated at " + System.currentTimeMillis());
        // }));
    }
}