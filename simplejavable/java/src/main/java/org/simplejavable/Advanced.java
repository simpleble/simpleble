package org.simplejavable;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.function.BiConsumer;
import java.util.function.BiPredicate;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.function.Supplier;

/**
 * Backend- and platform-specific APIs that are not part of the standard SimpleBLE API.
 * <p>
 * A platform method throws {@link OperationNotSupportedException} when the native library was not built for that
 * platform.
 */
public final class Advanced {
    private Advanced() {}

    /**
     * Pairing callbacks for peripherals found through a Dongl adapter.
     * <p>
     * Register them before calling {@link Peripheral#connect()}, as pairing can start as soon as the connection is
     * up. Passing null unregisters a callback.
     * <p>
     * NOTE: Callbacks run on an internal worker thread, possibly while {@code connect()} is still running, and they
     * can block while waiting for the user. Requests without a callback, or whose callback throws, are rejected.
     * Peripherals from other backends throw {@link OperationNotSupportedException}.
     */
    public static final class Dongl {
        private Dongl() {}

        /**
         * Registers a callback that returns the passkey shown by the peer.
         * <p>
         * The callback must return exactly six decimal digits, including leading zeroes. Returning null, or any
         * other value, rejects pairing.
         * @param peripheral A peripheral found through a Dongl adapter.
         * @param callback The callback, or null to unregister it.
         */
        public static void setPasskeyRequestCallback(Peripheral peripheral, Supplier<String> callback) {
            Objects.requireNonNull(peripheral, "peripheral");
            peripheral.passkeyRequestCallback = callback;
            nativeDonglSetPasskeyRequestCallback(peripheral.getAdapterId(), peripheral.getInstanceId(),
                callback != null);
        }

        /**
         * Registers a callback that receives a passkey for the user to enter on the peer.
         * <p>
         * The passkey is exactly six decimal digits, including leading zeroes; show it unchanged.
         * @param peripheral A peripheral found through a Dongl adapter.
         * @param callback The callback, or null to unregister it.
         */
        public static void setPasskeyDisplayCallback(Peripheral peripheral, Consumer<String> callback) {
            Objects.requireNonNull(peripheral, "peripheral");
            peripheral.passkeyDisplayCallback = callback;
            nativeDonglSetPasskeyDisplayCallback(peripheral.getAdapterId(), peripheral.getInstanceId(),
                callback != null);
        }

        /**
         * Registers a callback for numeric comparison during pairing.
         * <p>
         * The callback receives exactly six decimal digits, including leading zeroes. Return true only after the
         * user confirms that the peer shows the same number; return false to reject pairing.
         * @param peripheral A peripheral found through a Dongl adapter.
         * @param callback The callback, or null to unregister it.
         */
        public static void setNumericComparisonCallback(Peripheral peripheral, Predicate<String> callback) {
            Objects.requireNonNull(peripheral, "peripheral");
            peripheral.numericComparisonCallback = callback;
            nativeDonglSetNumericComparisonCallback(peripheral.getAdapterId(), peripheral.getInstanceId(),
                callback != null);
        }

        /**
         * Secures the connection with LE Secure Connections.
         * <p>
         * With mitm, the peer must also be authenticated through the passkey or numeric comparison callbacks. Pairing
         * started by this call creates no bond, and the Dongl disconnects if it fails.
         * @param peripheral A connected peripheral found through a Dongl adapter.
         * @param mitm Whether the peer must also be authenticated.
         */
        public static void secureConnection(Peripheral peripheral, boolean mitm) {
            Objects.requireNonNull(peripheral, "peripheral");
            nativeDonglSecureConnection(peripheral.getAdapterId(), peripheral.getInstanceId(), mitm);
        }
    }

    /**
     * macOS CoreBluetooth extensions.
     */
    public static final class MacOS {
        private MacOS() {}

        /**
         * Sets the local name included in a local peripheral's advertisement.
         * <p>
         * This changes only the advertisement, not the system Bluetooth name. Call before
         * {@link LocalPeripheral#start()}.
         * @param peripheral The local peripheral.
         * @param localName The name, or null to remove the override.
         */
        public static void setAdvertisementLocalName(LocalPeripheral peripheral, String localName) {
            Objects.requireNonNull(peripheral, "peripheral");
            nativeMacOSSetAdvertisementLocalName(peripheral.getInstanceId(), localName);
        }

        /**
         * Retrieves peripherals that CoreBluetooth can resolve from its system cache, without scanning.
         * <p>
         * A returned peripheral is not guaranteed to be reachable. Identifiers that are invalid or unknown to
         * CoreBluetooth are skipped. The adapter must be powered on.
         * @param adapter The adapter.
         * @param identifiers CoreBluetooth peripheral identifiers, as returned by {@link Peripheral#getAddress()}.
         * @return The peripherals that were found.
         */
        public static List<Peripheral> retrieveCachedPeripherals(Adapter adapter, List<String> identifiers) {
            Objects.requireNonNull(adapter, "adapter");
            String[] values = identifiers.stream()
                .map(identifier -> Objects.requireNonNull(identifier, "identifiers contains null"))
                .toArray(String[]::new);

            List<Peripheral> peripherals = new ArrayList<>();
            for (long peripheralId : nativeMacOSRetrieveCachedPeripherals(adapter.getInstanceId(), values)) {
                peripherals.add(adapter.getPeripheral(peripheralId));
            }
            return peripherals;
        }
    }

    /**
     * Linux BlueZ extensions.
     * <p>
     * The pairing callbacks apply to peripherals the adapter connects to and to clients of its local peripherals,
     * and receive the peer's address. Passkey and numeric comparison requests reach them when
     * {@link Config.SimpleBluez#setInteractivePairing(boolean)} is set before the first enumeration. Passing null
     * unregisters a callback.
     * <p>
     * NOTE: Pairing callbacks run on SimpleBLE's BlueZ event thread, which waits for them to return. Passkey and
     * numeric comparison requests without a callback, and requests whose callback throws, are rejected. Adapters
     * from other backends throw {@link OperationNotSupportedException}.
     */
    public static final class Linux {
        private Linux() {}

        /**
         * Sets the local name included in a local peripheral's BlueZ advertisement.
         * <p>
         * This changes only the advertisement, not the adapter alias. Call before {@link LocalPeripheral#start()}.
         * @param peripheral The local peripheral.
         * @param localName The name, or null to remove the override.
         */
        public static void setAdvertisementLocalName(LocalPeripheral peripheral, String localName) {
            Objects.requireNonNull(peripheral, "peripheral");
            nativeLinuxSetAdvertisementLocalName(peripheral.getInstanceId(), localName);
        }

        /**
         * Registers a callback that receives the peer's address and returns the passkey the peer requested.
         * <p>
         * The callback must return exactly six decimal digits, including leading zeroes. Returning null, or any
         * other value, rejects pairing.
         * @param adapter The adapter the peer pairs through.
         * @param callback The callback, or null to unregister it.
         */
        public static void setPasskeyRequestCallback(Adapter adapter, Function<BluetoothAddress, String> callback) {
            Objects.requireNonNull(adapter, "adapter");
            adapter.passkeyRequestCallback = callback;
            nativeLinuxSetPasskeyRequestCallback(adapter.getInstanceId(), callback != null);
        }

        /**
         * Registers a callback that receives the peer's address and a passkey for the user to enter on the peer.
         * <p>
         * The passkey is exactly six decimal digits, including leading zeroes; show it unchanged.
         * @param adapter The adapter the peer pairs through.
         * @param callback The callback, or null to unregister it.
         */
        public static void setPasskeyDisplayCallback(Adapter adapter, BiConsumer<BluetoothAddress, String> callback) {
            Objects.requireNonNull(adapter, "adapter");
            adapter.passkeyDisplayCallback = callback;
            nativeLinuxSetPasskeyDisplayCallback(adapter.getInstanceId(), callback != null);
        }

        /**
         * Registers a callback for numeric comparison while a peer pairs.
         * <p>
         * The callback receives the peer's address and exactly six decimal digits, including leading zeroes. Return
         * true only after the user confirms that the peer shows the same number; return false to reject pairing.
         * @param adapter The adapter the peer pairs through.
         * @param callback The callback, or null to unregister it.
         */
        public static void setNumericComparisonCallback(Adapter adapter,
                                                        BiPredicate<BluetoothAddress, String> callback) {
            Objects.requireNonNull(adapter, "adapter");
            adapter.numericComparisonCallback = callback;
            nativeLinuxSetNumericComparisonCallback(adapter.getInstanceId(), callback != null);
        }

        /**
         * Registers a callback that approves Just Works pairing with a peer.
         * <p>
         * The callback receives the peer's address. Return true to accept pairing and false to reject it. Without
         * this callback, Just Works pairing is accepted.
         * @param adapter The adapter the peer pairs through.
         * @param callback The callback, or null to unregister it.
         */
        public static void setPairingAuthorizationCallback(Adapter adapter, Predicate<BluetoothAddress> callback) {
            Objects.requireNonNull(adapter, "adapter");
            adapter.pairingAuthorizationCallback = callback;
            nativeLinuxSetPairingAuthorizationCallback(adapter.getInstanceId(), callback != null);
        }
    }

    private static native void nativeDonglSetPasskeyRequestCallback(long adapterId, long peripheralId, boolean enabled);
    private static native void nativeDonglSetPasskeyDisplayCallback(long adapterId, long peripheralId, boolean enabled);
    private static native void nativeDonglSetNumericComparisonCallback(long adapterId, long peripheralId,
                                                                       boolean enabled);
    private static native void nativeDonglSecureConnection(long adapterId, long peripheralId, boolean mitm);
    private static native void nativeMacOSSetAdvertisementLocalName(long peripheralId, String localName);
    private static native long[] nativeMacOSRetrieveCachedPeripherals(long adapterId, String[] identifiers);
    private static native void nativeLinuxSetAdvertisementLocalName(long peripheralId, String localName);
    private static native void nativeLinuxSetPasskeyRequestCallback(long adapterId, boolean enabled);
    private static native void nativeLinuxSetPasskeyDisplayCallback(long adapterId, boolean enabled);
    private static native void nativeLinuxSetNumericComparisonCallback(long adapterId, boolean enabled);
    private static native void nativeLinuxSetPairingAuthorizationCallback(long adapterId, boolean enabled);

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
