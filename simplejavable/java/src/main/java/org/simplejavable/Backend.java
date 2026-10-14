package org.simplejavable;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Represents a SimpleBLE backend, such as the operating system's Bluetooth stack or the Dongl USB adapter.
 * <p>
 * Use backends to choose where adapters come from. For example, {@link #getDongl()} gives only Dongl adapters.
 */
public class Backend {
    private static final Map<Long, Backend> backendCache = new ConcurrentHashMap<>();

    private final long instanceId;

    private Backend(long newInstanceId) {
        this.instanceId = newInstanceId;
    }

    /**
     * Gets the identifier of the backend: {@code "SimpleBluez"}, {@code "WinRT"}, {@code "CoreBluetooth"}, or
     * {@code "Dongl"}.
     * @return The string identifier.
     */
    public String getIdentifier() {
        String identifier = nativeBackendIdentifier(instanceId);
        return identifier != null ? identifier : "";
    }

    /**
     * Checks if Bluetooth is enabled for this backend.
     * @return True if Bluetooth is enabled.
     */
    public boolean isBluetoothEnabled() {
        return nativeBackendIsBluetoothEnabled(instanceId);
    }

    /**
     * Gets the adapters of this backend.
     * <p>
     * These are the same Adapter objects that {@link Adapter#getAdapters()} returns for the same native adapters.
     * @return A list of Adapter objects.
     */
    public List<Adapter> getAdapters() {
        List<Adapter> adapters = new ArrayList<>();
        for (long adapterId : nativeBackendGetAdapters(instanceId)) {
            adapters.add(Adapter.fromId(adapterId));
        }
        return adapters;
    }

    /**
     * Gets all active backends.
     * <p>
     * This initializes the backends. Set any {@link Config} values, such as
     * {@link Config.Dongl#setUseDonglBackend(boolean)}, before the first call.
     * @return A list of Backend objects.
     */
    public static List<Backend> getBackends() {
        List<Backend> backends = new ArrayList<>();
        for (long backendId : nativeGetBackends()) {
            backends.add(backendCache.computeIfAbsent(backendId, Backend::new));
        }
        return backends;
    }

    /**
     * Gets the backend of the operating system's Bluetooth stack.
     * @return The native backend.
     */
    public static Backend getNative() {
        return backendCache.computeIfAbsent(nativeGetNative(), Backend::new);
    }

    /**
     * Gets the Dongl backend, whether or not {@link Config.Dongl#setUseDonglBackend(boolean)} lists it in
     * {@link #getBackends()}.
     * @return The Dongl backend.
     * @throws UnsupportedOperationException if this build of SimpleBLE doesn't include the Dongl backend.
     */
    public static Backend getDongl() {
        return backendCache.computeIfAbsent(nativeGetDongl(), Backend::new);
    }

    /**
     * Gets the Simulator backend, which only has adapters while a simulation is active.
     * @return The Simulator backend.
     * @throws UnsupportedOperationException if this build of SimpleBLE doesn't include the Simulator backend.
     */
    public static Backend getSimulator() {
        return backendCache.computeIfAbsent(nativeGetSimulator(), Backend::new);
    }

    private static native long[] nativeGetBackends();
    private static native long nativeGetNative();
    private static native long nativeGetDongl();
    private static native long nativeGetSimulator();
    private native String nativeBackendIdentifier(long backendId);
    private native boolean nativeBackendIsBluetoothEnabled(long backendId);
    private native long[] nativeBackendGetAdapters(long backendId);

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
