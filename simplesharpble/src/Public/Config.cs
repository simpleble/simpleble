using SimpleSharpBLE.Internal.Interop;

namespace SimpleSharpBLE;

public enum AndroidConnectionPriority { Disabled = -1, Balanced = 0, High = 1, LowPower = 2, Dck = 3 }

/// <summary>Process-wide native configuration. Set all values before enumerating any backend or adapter.</summary>
public static class Config
{
    public static void ResetAll() => NativeMethods.simpleble_config_reset_all();
    public static class SimpleBluez
    {
        public static void Reset() => NativeMethods.simpleble_config_simplebluez_reset();
        public static bool UseSystemBus
        {
            get => NativeMethods.simpleble_config_simplebluez_get_use_system_bus();
            set
            {
                NativeMethods.simpleble_config_simplebluez_set_use_system_bus(value);
            }
        }
        public static TimeSpan ConnectionTimeout
        {
            get => TimeSpan.FromMilliseconds(NativeMethods.simpleble_config_simplebluez_get_connection_timeout_ms());
            set
            {
                if (value < TimeSpan.Zero || value.TotalMilliseconds > long.MaxValue / 1_000_000) throw new ArgumentOutOfRangeException(nameof(value));
                NativeMethods.simpleble_config_simplebluez_set_connection_timeout_ms(checked((long)Math.Ceiling(value.TotalMilliseconds)));
            }
        }
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
    public static class WinRT
    {
        public static void Reset() => NativeMethods.simpleble_config_winrt_reset();
        [Obsolete("SimpleBLE uses its own WinRT MTA apartment by default.")]
        public static bool ExperimentalUseOwnMtaApartment
        {
            get => NativeMethods.simpleble_config_winrt_get_experimental_use_own_mta_apartment();
            set
            {
                NativeMethods.simpleble_config_winrt_set_experimental_use_own_mta_apartment(value);
            }
        }
        public static bool ExperimentalReinitializeWinrtApartmentOnMainThread
        {
            get => NativeMethods.simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread();
            set
            {
                NativeMethods.simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(value);
            }
        }
        public static bool UseDeferredDisconnect
        {
            get => NativeMethods.simpleble_config_winrt_get_use_deferred_disconnect();
            set
            {
                NativeMethods.simpleble_config_winrt_set_use_deferred_disconnect(value);
            }
        }
    }
    public static class CoreBluetooth
    {
        public static void Reset() => NativeMethods.simpleble_config_corebluetooth_reset();
    }
    public static class Android
    {
        public static void Reset() => NativeMethods.simpleble_config_android_reset();
        public static AndroidConnectionPriority ConnectionPriority
        {
            get => NativeMethods.simpleble_config_android_get_connection_priority();
            set
            {
                if (!Enum.IsDefined(value)) throw new ArgumentOutOfRangeException(nameof(value));
                NativeMethods.simpleble_config_android_set_connection_priority(value);
            }
        }
    }
    public static class Dongl
    {
        public static void Reset() => NativeMethods.simpleble_config_dongl_reset();
        public static bool UseDonglBackend
        {
            get => NativeMethods.simpleble_config_dongl_get_use_dongl_backend();
            set
            {
                NativeMethods.simpleble_config_dongl_set_use_dongl_backend(value);
            }
        }
        public static bool AutoUpdate
        {
            get => NativeMethods.simpleble_config_dongl_get_auto_update();
            set
            {
                NativeMethods.simpleble_config_dongl_set_auto_update(value);
            }
        }
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

