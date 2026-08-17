#include <simpleble/Advanced.h>

#include "BuilderBase.h"
#include "PeripheralDongl.h"

namespace SimpleBLE::Advanced::Dongl {

void set_passkey_request_callback(Peripheral& peripheral, const std::function<std::optional<std::string>()>& callback) {
    Factory::get_internal<PeripheralDongl>(peripheral).set_passkey_request_callback(callback);
}

void set_passkey_display_callback(Peripheral& peripheral,
                                  const std::function<void(const std::string& passkey)>& callback) {
    Factory::get_internal<PeripheralDongl>(peripheral).set_passkey_display_callback(callback);
}

void set_numeric_comparison_callback(Peripheral& peripheral,
                                     const std::function<bool(const std::string& passkey)>& callback) {
    Factory::get_internal<PeripheralDongl>(peripheral).set_numeric_comparison_callback(callback);
}

}  // namespace SimpleBLE::Advanced::Dongl
