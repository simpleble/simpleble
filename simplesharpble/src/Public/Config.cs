using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

/// <summary>Connection priority requested from Android after connecting.</summary>
public enum AndroidConnectionPriority
{
    /// <summary>Do not request a priority.</summary>
    Disabled = -1,
    /// <summary>Android's default balance of throughput and power.</summary>
    Balanced = 0,
    /// <summary>Lower latency and higher throughput at higher power.</summary>
    High = 1,
    /// <summary>Lower power at higher latency.</summary>
    LowPower = 2,
    /// <summary>Digital Car Key priority, on Android 14 or newer.</summary>
    Dck = 3,
}

/// <summary>Process-wide native configuration. Set all values before enumerating any backend or adapter.</summary>
public static class Config
{
    /// <summary>Restores every backend's defaults.</summary>
    public static void ResetAll() => NativeMethods.simpleble_config_reset_all();
    /// <summary>Linux BlueZ backend options.</summary>
    public static class SimpleBluez
    {
        /// <summary>Restores the BlueZ defaults.</summary>
        public static void Reset() => NativeMethods.simpleble_config_simplebluez_reset();
        /// <summary>Connects to BlueZ over the D-Bus system bus rather than the session bus. Defaults to true.</summary>
        public static bool UseSystemBus
        {
            get => NativeMethods.simpleble_config_simplebluez_get_use_system_bus();
            set
            {
                NativeMethods.simpleble_config_simplebluez_set_use_system_bus(value);
            }
        }
        /// <summary>How long Connect waits for BlueZ. Defaults to 2 seconds.</summary>
        public static TimeSpan ConnectionTimeout
        {
            get => TimeSpan.FromMilliseconds(NativeMethods.simpleble_config_simplebluez_get_connection_timeout_ms());
            set
            {
                if (value < TimeSpan.Zero || value.TotalMilliseconds > long.MaxValue / 1_000_000) throw new ArgumentOutOfRangeException(nameof(value));
                NativeMethods.simpleble_config_simplebluez_set_connection_timeout_ms(checked((long)Math.Ceiling(value.TotalMilliseconds)));
            }
        }
        /// <summary>How long Disconnect waits for BlueZ. Defaults to 1 second.</summary>
        public static TimeSpan DisconnectionTimeout
        {
            get => TimeSpan.FromMilliseconds(NativeMethods.simpleble_config_simplebluez_get_disconnection_timeout_ms());
            set
            {
                if (value < TimeSpan.Zero || value.TotalMilliseconds > long.MaxValue / 1_000_000) throw new ArgumentOutOfRangeException(nameof(value));
                NativeMethods.simpleble_config_simplebluez_set_disconnection_timeout_ms(checked((long)Math.Ceiling(value.TotalMilliseconds)));
            }
        }
    }
    /// <summary>Windows backend options.</summary>
    public static class WinRT
    {
        /// <summary>Restores the Windows defaults.</summary>
        public static void Reset() => NativeMethods.simpleble_config_winrt_reset();
        /// <summary>Runs WinRT calls on SimpleBLE's own MTA thread. Defaults to true.</summary>
        [Obsolete("SimpleBLE uses its own WinRT MTA apartment by default.")]
        public static bool ExperimentalUseOwnMtaApartment
        {
            get => NativeMethods.simpleble_config_winrt_get_experimental_use_own_mta_apartment();
            set
            {
                NativeMethods.simpleble_config_winrt_set_experimental_use_own_mta_apartment(value);
            }
        }
        /// <summary>Experimental: reinitializes the WinRT apartment on the calling thread when the backend starts. Defaults to false.</summary>
        public static bool ExperimentalReinitializeWinrtApartmentOnMainThread
        {
            get => NativeMethods.simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread();
            set
            {
                NativeMethods.simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(value);
            }
        }
        /// <summary>Returns from Disconnect immediately and cleans up in the background instead of blocking. Defaults to true.</summary>
        public static bool UseDeferredDisconnect
        {
            get => NativeMethods.simpleble_config_winrt_get_use_deferred_disconnect();
            set
            {
                NativeMethods.simpleble_config_winrt_set_use_deferred_disconnect(value);
            }
        }
    }
    /// <summary>macOS and iOS backend options. There are none yet.</summary>
    public static class CoreBluetooth
    {
        /// <summary>Restores the CoreBluetooth defaults.</summary>
        public static void Reset() => NativeMethods.simpleble_config_corebluetooth_reset();
    }
    /// <summary>Android backend options.</summary>
    public static class Android
    {
        /// <summary>Restores the Android defaults.</summary>
        public static void Reset() => NativeMethods.simpleble_config_android_reset();
        /// <summary>Priority requested after each connection. Defaults to Disabled, which leaves Android's choice.</summary>
        public static AndroidConnectionPriority ConnectionPriority
        {
            get => NativeMethods.simpleble_config_android_get_connection_priority();
            set
            {
                if (!Enum.IsDefined(typeof(AndroidConnectionPriority), value)) throw new ArgumentOutOfRangeException(nameof(value));
                NativeMethods.simpleble_config_android_set_connection_priority(value);
            }
        }
    }
    /// <summary>Dongl backend options.</summary>
    public static class Dongl
    {
        /// <summary>Restores the Dongl defaults.</summary>
        public static void Reset() => NativeMethods.simpleble_config_dongl_reset();
        /// <summary>Adds the Dongl backend, and its USB adapters, to enumeration. Defaults to false.</summary>
        public static bool UseDonglBackend
        {
            get => NativeMethods.simpleble_config_dongl_get_use_dongl_backend();
            set
            {
                NativeMethods.simpleble_config_dongl_set_use_dongl_backend(value);
            }
        }
        /// <summary>Reserved for Dongl firmware updates; the current backend ignores it. Defaults to false.</summary>
        public static bool AutoUpdate
        {
            get => NativeMethods.simpleble_config_dongl_get_auto_update();
            set
            {
                NativeMethods.simpleble_config_dongl_set_auto_update(value);
            }
        }
        /// <summary>Reserved for Dongl firmware updates; the current backend ignores it. Defaults to false.</summary>
        public static bool ForceUpdate
        {
            get => NativeMethods.simpleble_config_dongl_get_force_update();
            set
            {
                NativeMethods.simpleble_config_dongl_set_force_update(value);
            }
        }
    }
}

