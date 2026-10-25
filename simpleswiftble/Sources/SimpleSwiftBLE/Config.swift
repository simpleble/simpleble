internal import CSimpleBLE

/// Process-wide settings, grouped by backend. Set them at startup, before the first call that reaches a backend.
public enum Config {
    /// Restores every setting's default.
    public static func resetAll() {
        simpleble_config_reset_all()
    }

    /// Settings of the Dongl backend.
    public enum Dongl {
        /// Whether `Adapter.getAdapters()` includes Dongl adapters. On by default.
        public static var useDonglBackend: Bool {
            get { simpleble_config_dongl_get_use_dongl_backend() }
            set { simpleble_config_dongl_set_use_dongl_backend(newValue) }
        }

        /// Whether SimpleBLE updates Dongl firmware automatically.
        public static var autoUpdate: Bool {
            get { simpleble_config_dongl_get_auto_update() }
            set { simpleble_config_dongl_set_auto_update(newValue) }
        }

        /// Whether SimpleBLE forces a Dongl firmware update.
        public static var forceUpdate: Bool {
            get { simpleble_config_dongl_get_force_update() }
            set { simpleble_config_dongl_set_force_update(newValue) }
        }

        /// Restores the Dongl settings' defaults.
        public static func reset() {
            simpleble_config_dongl_reset()
        }
    }
}
