#pragma once

#include <simpleble/Logging.h>

#include <exception>
#include <string>
#include <utility>

namespace Org {
namespace SimpleKtBLE {

template <typename Func>
void invoke_callback(const char* name, Func&& func) noexcept {
    try {
        std::forward<Func>(func)();
    } catch (const std::exception& exception) {
        ::SimpleBLE::Logging::Logger::get()->log(
            ::SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__, __LINE__, name,
            std::string("Java callback failed: ") + exception.what());
    } catch (...) {
        ::SimpleBLE::Logging::Logger::get()->log(::SimpleBLE::Logging::Level::Error, "SimpleKtBLE", __FILE__,
                                                 __LINE__, name, "Java callback failed with an unknown error");
    }
}

}  // namespace SimpleKtBLE
}  // namespace Org
