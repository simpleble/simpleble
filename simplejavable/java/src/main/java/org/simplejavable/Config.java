package org.simplejavable;

import java.time.Duration;
import java.util.Objects;

/**
 * Process-wide configuration of the SimpleBLE backends.
 * <p>
 * Set all values before the first call to {@link Adapter#getAdapters()} or {@link Backend#getBackends()}.
 * Changes made after a backend or adapter has been created may not take effect.
 */
public final class Config {
    private Config() {}

    /**
     * Restores the defaults of every backend.
     */
    public static void resetAll() {
        nativeResetAll();
    }

    /**
     * Options of the Linux backend, which uses BlueZ.
     */
    public static final class SimpleBluez {
        private SimpleBluez() {}

        /**
         * Restores the BlueZ defaults.
         */
        public static void reset() {
            nativeSimpleBluezReset();
        }

        /**
         * Checks whether BlueZ is reached over the D-Bus system bus rather than the session bus. Defaults to true.
         * @return True if the system bus is used.
         */
        public static boolean getUseSystemBus() {
            return nativeSimpleBluezGetUseSystemBus();
        }

        /**
         * Selects the D-Bus system bus (true) or session bus (false) to reach BlueZ.
         * @param value True to use the system bus.
         */
        public static void setUseSystemBus(boolean value) {
            nativeSimpleBluezSetUseSystemBus(value);
        }

        /**
         * Gets how long {@link Peripheral#connect()} waits for BlueZ to connect. Defaults to 2 seconds.
         * @return The connection timeout.
         */
        public static Duration getConnectionTimeout() {
            return Duration.ofNanos(nativeSimpleBluezGetConnectionTimeout());
        }

        /**
         * Sets how long {@link Peripheral#connect()} waits for BlueZ to connect.
         * @param timeout A non-negative timeout.
         */
        public static void setConnectionTimeout(Duration timeout) {
            nativeSimpleBluezSetConnectionTimeout(toNanos(timeout));
        }

        /**
         * Gets how long {@link Peripheral#disconnect()} waits for BlueZ to disconnect. Defaults to 1 second.
         * @return The disconnection timeout.
         */
        public static Duration getDisconnectionTimeout() {
            return Duration.ofNanos(nativeSimpleBluezGetDisconnectionTimeout());
        }

        /**
         * Sets how long {@link Peripheral#disconnect()} waits for BlueZ to disconnect.
         * @param timeout A non-negative timeout.
         */
        public static void setDisconnectionTimeout(Duration timeout) {
            nativeSimpleBluezSetDisconnectionTimeout(toNanos(timeout));
        }
    }

    /**
     * Options of the Windows backend.
     */
    public static final class WinRT {
        private WinRT() {}

        /**
         * Restores the Windows defaults.
         */
        public static void reset() {
            nativeWinRTReset();
        }

        /**
         * Checks whether WinRT calls run on SimpleBLE's own MTA apartment. Defaults to true.
         * @return True if SimpleBLE's own apartment is used.
         * @deprecated SimpleBLE uses its own MTA apartment by default. This flag will be removed in a future release.
         */
        @Deprecated
        public static boolean getExperimentalUseOwnMtaApartment() {
            return nativeWinRTGetExperimentalUseOwnMtaApartment();
        }

        /**
         * Selects whether WinRT calls run on SimpleBLE's own MTA apartment.
         * @param value True to use SimpleBLE's own apartment.
         * @deprecated SimpleBLE uses its own MTA apartment by default. This flag will be removed in a future release.
         */
        @Deprecated
        public static void setExperimentalUseOwnMtaApartment(boolean value) {
            nativeWinRTSetExperimentalUseOwnMtaApartment(value);
        }

        /**
         * Checks whether the WinRT apartment is reinitialized on the calling thread when the backend starts.
         * Experimental. Defaults to false.
         * @return True if the apartment is reinitialized.
         */
        public static boolean getExperimentalReinitializeWinrtApartmentOnMainThread() {
            return nativeWinRTGetExperimentalReinitializeWinrtApartmentOnMainThread();
        }

        /**
         * Selects whether the WinRT apartment is reinitialized on the calling thread when the backend starts.
         * Experimental.
         * @param value True to reinitialize the apartment.
         */
        public static void setExperimentalReinitializeWinrtApartmentOnMainThread(boolean value) {
            nativeWinRTSetExperimentalReinitializeWinrtApartmentOnMainThread(value);
        }

        /**
         * Checks whether {@link Peripheral#disconnect()} returns immediately and cleans up in the background.
         * Defaults to true.
         * @return True if disconnections are deferred.
         */
        public static boolean getUseDeferredDisconnect() {
            return nativeWinRTGetUseDeferredDisconnect();
        }

        /**
         * Selects whether {@link Peripheral#disconnect()} returns immediately and cleans up in the background.
         * @param value True to defer disconnections.
         */
        public static void setUseDeferredDisconnect(boolean value) {
            nativeWinRTSetUseDeferredDisconnect(value);
        }
    }

    /**
     * Options of the macOS backend. There are none yet.
     */
    public static final class CoreBluetooth {
        private CoreBluetooth() {}

        /**
         * Restores the CoreBluetooth defaults.
         */
        public static void reset() {
            nativeCoreBluetoothReset();
        }
    }

    /**
     * Options of the Android backend.
     */
    public static final class Android {
        private Android() {}

        /** Restores the Android defaults. */
        public static void reset() {
            nativeAndroidReset();
        }

        /** Negotiate MTU on connect. Defaults to true. */
        public static boolean getNegotiateMtu() {
            return nativeAndroidGetNegotiateMtu();
        }

        public static void setNegotiateMtu(boolean value) {
            nativeAndroidSetNegotiateMtu(value);
        }
    }

    /**
     * Options of the Dongl backend.
     */
    public static final class Dongl {
        private Dongl() {}

        /**
         * Restores the Dongl defaults.
         */
        public static void reset() {
            nativeDonglReset();
        }

        /**
         * Checks whether the Dongl backend, and its USB adapters, take part in enumeration. Defaults to true.
         * @return True if the Dongl backend is enabled.
         */
        public static boolean getUseDonglBackend() {
            return nativeDonglGetUseDonglBackend();
        }

        /**
         * Enables or disables the Dongl backend. Change it before the first enumeration.
         * @param value True to enable the Dongl backend.
         */
        public static void setUseDonglBackend(boolean value) {
            nativeDonglSetUseDonglBackend(value);
        }

        /**
         * Reserved for Dongl firmware updates; the current backend ignores it. Defaults to false.
         * @return The stored value.
         */
        public static boolean getAutoUpdate() {
            return nativeDonglGetAutoUpdate();
        }

        /**
         * Reserved for Dongl firmware updates; the current backend ignores it.
         * @param value The value to store.
         */
        public static void setAutoUpdate(boolean value) {
            nativeDonglSetAutoUpdate(value);
        }

        /**
         * Reserved for Dongl firmware updates; the current backend ignores it. Defaults to false.
         * @return The stored value.
         */
        public static boolean getForceUpdate() {
            return nativeDonglGetForceUpdate();
        }

        /**
         * Reserved for Dongl firmware updates; the current backend ignores it.
         * @param value The value to store.
         */
        public static void setForceUpdate(boolean value) {
            nativeDonglSetForceUpdate(value);
        }
    }

    private static long toNanos(Duration timeout) {
        Objects.requireNonNull(timeout, "timeout");
        if (timeout.isNegative()) {
            throw new IllegalArgumentException("The timeout cannot be negative.");
        }
        return timeout.toNanos();
    }

    private static native void nativeResetAll();
    private static native void nativeSimpleBluezReset();
    private static native boolean nativeSimpleBluezGetUseSystemBus();
    private static native void nativeSimpleBluezSetUseSystemBus(boolean value);
    private static native long nativeSimpleBluezGetConnectionTimeout();
    private static native void nativeSimpleBluezSetConnectionTimeout(long nanos);
    private static native long nativeSimpleBluezGetDisconnectionTimeout();
    private static native void nativeSimpleBluezSetDisconnectionTimeout(long nanos);
    private static native void nativeWinRTReset();
    private static native boolean nativeWinRTGetExperimentalUseOwnMtaApartment();
    private static native void nativeWinRTSetExperimentalUseOwnMtaApartment(boolean value);
    private static native boolean nativeWinRTGetExperimentalReinitializeWinrtApartmentOnMainThread();
    private static native void nativeWinRTSetExperimentalReinitializeWinrtApartmentOnMainThread(boolean value);
    private static native boolean nativeWinRTGetUseDeferredDisconnect();
    private static native void nativeWinRTSetUseDeferredDisconnect(boolean value);
    private static native void nativeCoreBluetoothReset();
    private static native void nativeAndroidReset();
    private static native boolean nativeAndroidGetNegotiateMtu();
    private static native void nativeAndroidSetNegotiateMtu(boolean value);
    private static native void nativeDonglReset();
    private static native boolean nativeDonglGetUseDonglBackend();
    private static native void nativeDonglSetUseDonglBackend(boolean value);
    private static native boolean nativeDonglGetAutoUpdate();
    private static native void nativeDonglSetAutoUpdate(boolean value);
    private static native boolean nativeDonglGetForceUpdate();
    private static native void nativeDonglSetForceUpdate(boolean value);

    static {
        NativeLibraryLoader.ensureLoaded();
    }
}
