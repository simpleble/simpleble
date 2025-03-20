#pragma once

namespace SimpleBLE {
namespace Config {
    namespace SimpleBluez {
        static void reset() {}
    }

    namespace WinRT {
        inline static bool experimental_use_own_mta_apartment = false;

        static void reset() {
            experimental_use_own_mta_apartment = false;
        }
    }

    namespace CoreBluetooth {
        static void reset() {}
    }

    namespace Android {
        static void reset() {}
    }

    namespace Base {
        static void reset_all() {
            SimpleBluez::reset();
            WinRT::reset();
            CoreBluetooth::reset();
            Android::reset();
        }
    }
}  // namespace Config
}  // namespace SimpleBLE
